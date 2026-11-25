#include "http_api.hpp"

#include <httplib.h>

#include <chrono>
#include <nlohmann/json.hpp>

#include "log.hpp"

namespace dash {

using nlohmann::json;
using namespace std::chrono_literals;

namespace {

constexpr auto kKeepAlive = 15s; // an idle event stream sends a comment this often

void send_json(httplib::Response &res, int status, const json &body) {
  res.status = status;
  res.set_content(body.dump(), "application/json");
}

void send_error(httplib::Response &res, int status, const std::string &message) {
  send_json(res, status, {{"error", message}});
}

std::string sse_updates(const std::vector<std::string> &messages) {
  std::string out;
  for (const auto &m : messages) out += "event: update\ndata: " + m + "\n\n";
  return out;
}

} // namespace

struct HttpServer::Impl {
  httplib::Server server;
  std::chrono::steady_clock::time_point started = std::chrono::steady_clock::now();
};

HttpServer::HttpServer(Hub &hub, Lights &lights, Display &display, SettingsStore &settings, Spotify *spotify,
                       const std::filesystem::path &web_dir, const std::filesystem::path &photos_dir)
    : impl_(std::make_unique<Impl>()) {
  auto &server = impl_->server;
  auto *impl = impl_.get();

  // Few threads: one kiosk keeps one event stream open, the rest are short requests.
  server.new_task_queue = [] { return new httplib::ThreadPool(4, 8); };

  // The frontend (web/dist). Hashed assets never change; index.html must be fresh.
  if (!server.set_mount_point("/", web_dir.string())) {
    log_warning("http: no frontend in {}, serving /api only", web_dir.string());
  }
  server.set_file_request_handler([](const httplib::Request &req, httplib::Response &res) {
    res.set_header("Cache-Control", req.path.starts_with("/assets/") ? "public, max-age=31536000, immutable" : "no-cache");
  });

  server.Get("/api/state", [&hub](const httplib::Request &, httplib::Response &res) {
    send_json(res, 200, hub.snapshot());
  });

  server.Get("/api/health", [impl](const httplib::Request &, httplib::Response &res) {
    const auto up = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - impl->started);
    send_json(res, 200, {{"ok", true}, {"uptimeS", up.count()}});
  });

  // Server-Sent Events: first every topic once (so nothing is lost between /api/state and
  // this stream), then each change as it comes.
  server.Get("/api/events", [&hub](const httplib::Request &, httplib::Response &res) {
    res.set_header("Cache-Control", "no-cache");
    struct Stream {
      std::uint64_t seq = 0;
      bool started = false;
    };
    auto stream = std::make_shared<Stream>();
    res.set_chunked_content_provider("text/event-stream", [&hub, stream](std::size_t, httplib::DataSink &sink) {
      std::string out;
      if (!stream->started) {
        stream->started = true;
        out = "retry: 2000\n\n" + sse_updates(hub.snapshot_messages(stream->seq));
      } else {
        const auto changes = hub.wait(stream->seq, kKeepAlive);
        if (changes.closed) {
          sink.done();
          return true;
        }
        if (changes.resync) {
          out = sse_updates(hub.snapshot_messages(stream->seq));
        } else if (changes.messages.empty()) {
          out = ": keep-alive\n\n";
        } else {
          out = sse_updates(changes.messages);
        }
      }
      return sink.write(out.data(), out.size()); // false: the page went away
    });
  });

  // Cached Immich previews (immich.hpp). The name is the asset id, so a file never changes.
  server.Get(R"(/api/photos/([0-9a-f-]{36}\.(jpg|webp)))", [photos_dir](const httplib::Request &req,
                                                                     httplib::Response &res) {
    const auto file = photos_dir / req.matches[1].str();
    if (!std::filesystem::is_regular_file(file)) return send_error(res, 404, "no such photo");
    res.set_file_content(file.string(), req.matches[2] == "jpg" ? "image/jpeg" : "image/webp");
    res.set_header("Cache-Control", "public, max-age=31536000, immutable");
  });

  server.Post("/api/lights/:id", [&lights](const httplib::Request &req, httplib::Response &res) {
    const json body = json::parse(req.body, nullptr, false);
    if (body.is_discarded()) return send_error(res, 400, "the body is not JSON");
    std::string error;
    switch (lights.apply(req.path_params.at("id"), body, error)) {
    case Lights::Result::ok:
      res.status = 204;
      break;
    case Lights::Result::unknown_light:
      send_error(res, 404, "no such light");
      break;
    case Lights::Result::bad_request:
      send_error(res, 400, error);
      break;
    }
  });

  server.Post("/api/settings", [&settings](const httplib::Request &req, httplib::Response &res) {
    const json body = json::parse(req.body, nullptr, false);
    if (body.is_discarded()) return send_error(res, 400, "the body is not JSON");
    if (std::string error = settings.apply(body); !error.empty()) return send_error(res, 400, error);
    res.status = 204;
  });

  server.Post("/api/display", [&display](const httplib::Request &req, httplib::Response &res) {
    const json body = json::parse(req.body, nullptr, false);
    const auto brightness = body.is_object() ? body.find("brightness") : body.end();
    if (body.is_discarded() || brightness == body.end() || !brightness->is_number()) {
      return send_error(res, 400, R"(expected {"brightness": 0-100})");
    }
    if (display.set_brightness(static_cast<int>(std::lround(brightness->get<double>())))) {
      res.status = 204;
    } else {
      send_error(res, 500, "the backlight could not be set");
    }
  });

  // The Spotify library and devices (spotify.hpp). Registered before /api/music/:action.
  auto spotify_result = [](httplib::Response &res, Spotify::Result result) {
    switch (result) {
    case Spotify::Result::ok:
      res.status = 204;
      break;
    case Spotify::Result::not_signed_in:
      send_error(res, 503, "Spotify is not signed in (tools/spotify-login.py)");
      break;
    case Spotify::Result::no_device:
      send_error(res, 409, "no Spotify device is active");
      break;
    case Spotify::Result::failed:
      send_error(res, 502, "Spotify refused or did not answer");
      break;
    }
  };
  auto body_field = [](const httplib::Request &req, const char *key) -> json {
    const json body = json::parse(req.body, nullptr, false);
    return body.is_object() && body.contains(key) ? body[key] : json(nullptr);
  };

  server.Get("/api/music/devices", [spotify](const httplib::Request &, httplib::Response &res) {
    if (!spotify) return send_error(res, 503, "Spotify is not configured");
    const json devices = spotify->devices();
    if (devices.is_null()) return send_error(res, 502, "Spotify did not list the devices");
    send_json(res, 200, devices);
  });

  server.Get("/api/music/tracks", [spotify](const httplib::Request &req, httplib::Response &res) {
    const std::string uri = req.get_param_value("uri");
    if (!uri.starts_with("spotify:")) return send_error(res, 400, "expected ?uri=spotify:...");
    if (!spotify) return send_error(res, 503, "Spotify is not configured");
    const auto number = [&](const char *name, int fallback) {
      try {
        return req.has_param(name) ? std::stoi(req.get_param_value(name)) : fallback;
      } catch (const std::exception &) {
        return fallback;
      }
    };
    const json tracks = spotify->tracks(uri, number("offset", 0), number("limit", 50));
    if (tracks.is_null()) return send_error(res, 502, "Spotify did not give the tracks");
    send_json(res, 200, tracks);
  });

  server.Post("/api/music/context", [spotify, spotify_result, body_field](const httplib::Request &req,
                                                                        httplib::Response &res) {
    const json uri = body_field(req, "uri");
    if (!uri.is_string() || !uri.get<std::string>().starts_with("spotify:")) {
      return send_error(res, 400, R"(expected {"uri": "spotify:..."})");
    }
    if (!spotify) return send_error(res, 503, "Spotify is not configured");
    const json device = body_field(req, "deviceId");
    const json offset = body_field(req, "offset");
    spotify_result(res, spotify->play_context(uri.get<std::string>(), device.is_string() ? device.get<std::string>() : "",
                                              offset.is_string() ? offset.get<std::string>() : ""));
  });

  server.Post("/api/music/device", [spotify, spotify_result, body_field](const httplib::Request &req,
                                                                       httplib::Response &res) {
    const json id = body_field(req, "id");
    if (!id.is_string() || id.get<std::string>().empty()) return send_error(res, 400, R"(expected {"id": "..."})");
    if (!spotify) return send_error(res, 503, "Spotify is not configured");
    spotify_result(res, spotify->transfer(id.get<std::string>()));
  });

  server.Post("/api/music/volume", [spotify, spotify_result, body_field](const httplib::Request &req,
                                                                       httplib::Response &res) {
    const json percent = body_field(req, "percent");
    if (!percent.is_number()) return send_error(res, 400, R"(expected {"percent": 0-100})");
    if (!spotify) return send_error(res, 503, "Spotify is not configured");
    spotify_result(res, spotify->volume(static_cast<int>(std::lround(percent.get<double>()))));
  });

  server.Post("/api/music/:action", [spotify](const httplib::Request &req, httplib::Response &res) {
    const std::string &action = req.path_params.at("action");
    if (action != "play" && action != "pause" && action != "next" && action != "previous") {
      return send_error(res, 404, "no such action");
    }
    if (!spotify) return send_error(res, 503, "Spotify is not configured (spotify.clientId in config.json)");
    switch (spotify->command(action)) {
    case Spotify::Result::ok:
      res.status = 204;
      break;
    case Spotify::Result::not_signed_in:
      send_error(res, 503, "Spotify is not signed in (tools/spotify-login.py)");
      break;
    case Spotify::Result::no_device:
      send_error(res, 409, "no Spotify device is active");
      break;
    case Spotify::Result::failed:
      send_error(res, 502, "Spotify refused or did not answer");
      break;
    }
  });

  server.set_exception_handler([](const httplib::Request &req, httplib::Response &res, std::exception_ptr ep) {
    std::string what = "unknown error";
    try {
      std::rethrow_exception(ep);
    } catch (const std::exception &e) {
      what = e.what();
    } catch (...) {
    }
    log_error("http: {} {}: {}", req.method, req.path, what);
    send_error(res, 500, what);
  });
}

HttpServer::~HttpServer() = default;

bool HttpServer::bind(const std::string &host, int port) { return impl_->server.bind_to_port(host, port); }

void HttpServer::run() { impl_->server.listen_after_bind(); }

void HttpServer::stop() { impl_->server.stop(); }

} // namespace dash
