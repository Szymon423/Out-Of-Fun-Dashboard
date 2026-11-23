#include "spotify.hpp"

#include <algorithm>
#include <cstdlib>
#include <fstream>

#include "fetch.hpp"
#include "log.hpp"

namespace dash {

using nlohmann::json;
using namespace std::chrono_literals;
namespace fs = std::filesystem;

namespace {

constexpr const char *kApi = "https://api.spotify.com/v1";
constexpr const char *kTokenUrl = "https://accounts.spotify.com/api/token";
constexpr auto kPlayingPoll = 5s;
constexpr auto kIdlePoll = 20s;
constexpr auto kNoLoginPoll = 30s;
constexpr auto kAfterCommand = 700ms; // Spotify needs a moment before the player shows a change
constexpr std::int64_t kDriftMs = 2500;

std::int64_t now_ms() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
      .count();
}

std::string text(const json &object, const char *key) {
  const auto it = object.find(key);
  return it != object.end() && it->is_string() ? it->get<std::string>() : std::string();
}

std::int64_t integer(const json &object, const char *key) {
  const auto it = object.find(key);
  return it != object.end() && it->is_number() ? it->get<std::int64_t>() : 0;
}

/** The cover closest to 640 px: the largest not above it, else the smallest there is. */
json cover(const json &images) {
  if (!images.is_array() || images.empty()) return nullptr;
  const json *best = nullptr;
  for (const json &image : images) {
    const auto width = integer(image, "width");
    if (width <= 640 && (!best || width > integer(*best, "width"))) best = &image;
  }
  if (!best) best = &images.back(); // Spotify lists the largest first
  const std::string url = text(*best, "url");
  return url.empty() ? json(nullptr) : json(url);
}

} // namespace

json now_playing(const json &player, std::int64_t now) {
  const auto item = player.find("item");
  if (item == player.end() || !item->is_object()) return nullptr; // nothing, or an advert

  const bool episode = text(*item, "type") == "episode";
  std::string artist;
  std::string album;
  json images;
  if (episode) {
    const json show = item->value("show", json::object());
    artist = text(show, "publisher");
    album = text(show, "name");
    images = item->contains("images") ? (*item)["images"] : show.value("images", json::array());
  } else {
    for (const json &a : item->value("artists", json::array())) {
      if (!artist.empty()) artist += ", ";
      artist += text(a, "name");
    }
    const json album_json = item->value("album", json::object());
    album = text(album_json, "name");
    images = album_json.value("images", json::array());
  }

  return {
      {"title", text(*item, "name")},
      {"uri", text(*item, "uri")},
      {"artist", artist},
      {"album", album},
      {"artUrl", cover(images)},
      {"isPlaying", player.value("is_playing", false)},
      {"progressMs", integer(player, "progress_ms")},
      {"readAt", now},
      {"durationMs", integer(*item, "duration_ms")},
      {"device", player.contains("device") ? text(player["device"], "name") : std::string()},
      // Phones and some speakers do not let the API set their volume (supports_volume false).
      {"volume", player.contains("device") && player["device"].value("supports_volume", true) &&
                         player["device"].contains("volume_percent") && player["device"]["volume_percent"].is_number()
                     ? player["device"]["volume_percent"]
                     : json(nullptr)},
  };
}

namespace {

std::string tracks_label(std::int64_t n) {
  const std::int64_t last = n % 10;
  const std::int64_t last_two = n % 100;
  const char *word = n == 1 ? "utwór" : last >= 2 && last <= 4 && !(last_two >= 12 && last_two <= 14) ? "utwory" : "utworów";
  return std::format("{} {}", n, word);
}

} // namespace

json music_library(const json &playlists, const json &recent, const std::string &user_id) {
  json lists = json::array();
  for (const json &p : playlists.value("items", json::array())) {
    if (!p.is_object() || text(p, "uri").empty()) continue;
    // The count is under "items" in newer answers, under "tracks" in older ones.
    const json tracks = p.contains("items") && p["items"].is_object() ? p["items"] : p.value("tracks", json::object());
    lists.push_back({{"name", text(p, "name")},
                     {"uri", text(p, "uri")},
                     {"imageUrl", cover(p.value("images", json::array()))},
                     {"detail", tracks.contains("total") ? tracks_label(integer(tracks, "total")) : std::string()}});
  }

  json albums = json::array();
  std::vector<std::string> seen;
  for (const json &item : recent.value("items", json::array())) {
    if (albums.size() >= 8) break;
    const json album = item.value("track", json::object()).value("album", json::object());
    const std::string uri = text(album, "uri");
    if (uri.empty() || std::ranges::find(seen, uri) != seen.end()) continue;
    seen.push_back(uri);
    const json artists = album.value("artists", json::array());
    albums.push_back({{"name", text(album, "name")},
                      {"uri", uri},
                      {"imageUrl", cover(album.value("images", json::array()))},
                      {"detail", artists.empty() ? std::string() : text(artists[0], "name")}});
  }

  return {{"playlists", lists},
          {"recent", albums},
          {"likedUri", user_id.empty() ? json(nullptr) : json("spotify:user:" + user_id + ":collection")}};
}

json track_list(const json &items) {
  json out = json::array();
  for (const json &item : items) {
    // Playlist items (/playlists/{id}/items) wrap the track in "item", older answers and the
    // liked songs in "track"; album tracks come bare.
    const json &track = item.contains("item") ? item["item"] : item.contains("track") ? item["track"] : item;
    if (!track.is_object() || track.value("is_local", false) || text(track, "uri").empty()) continue;
    std::string artist;
    for (const json &a : track.value("artists", json::array())) {
      if (!artist.empty()) artist += ", ";
      artist += text(a, "name");
    }
    out.push_back({{"name", text(track, "name")},
                   {"artist", artist},
                   {"uri", text(track, "uri")},
                   {"durationMs", integer(track, "duration_ms")}});
  }
  return out;
}

bool worth_publishing(const json &shown, const json &next) {
  if (shown.is_null() || next.is_null()) return shown.is_null() != next.is_null();
  for (const char *key : {"title", "artist", "album", "artUrl", "isPlaying", "durationMs", "device", "volume"}) {
    if (shown[key] != next[key]) return true;
  }
  // Where the page thinks the track is now, against where Spotify says it is.
  const std::int64_t elapsed = shown["isPlaying"].get<bool>() ? integer(next, "readAt") - integer(shown, "readAt") : 0;
  const std::int64_t expected = integer(shown, "progressMs") + elapsed;
  return std::llabs(expected - integer(next, "progressMs")) > kDriftMs;
}

Spotify::Spotify(Hub &hub, std::string client_id, fs::path token_file)
    : hub_(hub), client_id_(std::move(client_id)), token_file_(std::move(token_file)),
      thread_([this](std::stop_token stop) { run(stop); }) {}

Spotify::~Spotify() {
  thread_.request_stop();
  wake_.notify_all();
}

void Spotify::load_refresh_token() {
  // Called with token_mutex_ held.
  try {
    if (!fs::exists(token_file_)) return;
    const json saved = json::parse(std::ifstream(token_file_));
    refresh_token_ = saved.value("refreshToken", "");
    if (!refresh_token_.empty()) log_info("spotify: signed in ({})", token_file_.string());
  } catch (const std::exception &e) {
    log_warning("spotify: cannot read {}: {}", token_file_.string(), e.what());
  }
}

void Spotify::save_refresh_token(const std::string &token) {
  // Called with token_mutex_ held. Owner-only: the token gives control of the account.
  const auto tmp = fs::path(token_file_).concat(".tmp");
  {
    std::ofstream out(tmp);
    out << json{{"refreshToken", token}}.dump(2) << '\n';
    if (!out) {
      log_warning("spotify: cannot write {}", tmp.string());
      return;
    }
  }
  std::error_code ec;
  fs::permissions(tmp, fs::perms::owner_read | fs::perms::owner_write, ec);
  fs::rename(tmp, token_file_, ec);
  if (ec) log_warning("spotify: cannot save {}: {}", token_file_.string(), ec.message());
  token_file_time_ = fs::last_write_time(token_file_, ec); // our own write is not a new login
}

std::string Spotify::access_token() {
  const std::lock_guard lock(token_mutex_);
  // A changed file is a new login (tools/spotify-login.py): drop the old one and use it.
  std::error_code ec;
  if (const auto time = fs::last_write_time(token_file_, ec); !ec && time != token_file_time_) {
    token_file_time_ = time;
    refresh_token_.clear();
    access_token_.clear();
    library_stale_ = true;
  }
  if (refresh_token_.empty()) load_refresh_token();
  if (refresh_token_.empty()) return {};
  if (!access_token_.empty() && std::chrono::steady_clock::now() < access_expires_) return access_token_;

  try {
    const auto response = http_fetch({
        .url = kTokenUrl,
        .headers = {},
        .method = "POST",
        .body = form_body({{"grant_type", "refresh_token"}, {"refresh_token", refresh_token_}, {"client_id", client_id_}}),
        .content_type = "application/x-www-form-urlencoded",
        .timeout = 20s,
    });
    const json answer = json::parse(response.body);
    access_token_ = answer.at("access_token").get<std::string>();
    // Renew a minute early, so a token never runs out in the middle of a request.
    access_expires_ = std::chrono::steady_clock::now() + std::chrono::seconds(answer.value("expires_in", 3600)) - 60s;
    if (const std::string renewed = answer.value("refresh_token", ""); !renewed.empty() && renewed != refresh_token_) {
      refresh_token_ = renewed;
      save_refresh_token(renewed);
    }
    return access_token_;
  } catch (const FetchError &e) {
    if (e.status == 400) { // invalid_grant: the login was revoked or has expired
      log_error("spotify: the login is no longer valid, sign in again with tools/spotify-login.py: {}", e.what());
      refresh_token_.clear();
      access_token_.clear();
      std::error_code ec;
      fs::remove(token_file_, ec);
      return {};
    }
    throw;
  }
}

HttpResponse Spotify::api(const std::string &method, const std::string &path, const std::string &json_body) {
  for (int attempt = 0;; ++attempt) {
    const std::string token = access_token();
    if (token.empty()) throw NotSignedIn{};
    try {
      const bool sends = method != "GET";
      return http_fetch({.url = std::string(kApi) + path,
                         .headers = {"Authorization: Bearer " + token},
                         .method = method,
                         .body = json_body,
                         .content_type = sends && !json_body.empty() ? "application/json" : "",
                         .timeout = 10s});
    } catch (const FetchError &e) {
      if (e.status != 401 || attempt > 0) throw;
      const std::lock_guard lock(token_mutex_); // the access token ran out early: renew, retry
      access_token_.clear();
    }
  }
}

void Spotify::poke() {
  {
    const std::lock_guard lock(wake_mutex_);
    poke_ = true;
  }
  wake_.notify_all();
}

Spotify::Result Spotify::act(std::string_view what, const std::string &method, const std::string &path,
                             const std::string &body) {
  try {
    api(method, path, body);
    poke();
    return Result::ok;
  } catch (const NotSignedIn &) {
    return Result::not_signed_in;
  } catch (const FetchError &e) {
    if (e.status == 404) return Result::no_device; // NO_ACTIVE_DEVICE
    log_warning("spotify: {} failed: {}", what, e.what());
    return Result::failed;
  } catch (const std::exception &e) {
    log_warning("spotify: {} failed: {}", what, e.what());
    return Result::failed;
  }
}

Spotify::Result Spotify::command(std::string_view action) {
  if (action == "play") return act(action, "PUT", "/me/player/play");
  if (action == "pause") return act(action, "PUT", "/me/player/pause");
  if (action == "next") return act(action, "POST", "/me/player/next");
  if (action == "previous") return act(action, "POST", "/me/player/previous");
  return Result::failed;
}

Spotify::Result Spotify::play_context(const std::string &uri, const std::string &device_id,
                                      const std::string &offset_uri) {
  const std::string path = device_id.empty() ? "/me/player/play" : "/me/player/play?device_id=" + url_encode(device_id);
  json body = {{"context_uri", uri}};
  if (!offset_uri.empty()) body["offset"] = {{"uri", offset_uri}};
  return act("play " + uri, "PUT", path, body.dump());
}

json Spotify::tracks(const std::string &uri) {
  // spotify:playlist:<id>, spotify:album:<id>, spotify:user:<user>:collection
  const auto colon = uri.rfind(':');
  const std::string id = uri.substr(colon + 1);
  std::string path;
  if (uri.starts_with("spotify:playlist:")) {
    path = "/playlists/" + url_encode(id) + "/items?limit=100";
  } else if (uri.starts_with("spotify:album:")) {
    path = "/albums/" + url_encode(id) + "/tracks?limit=50";
  } else if (uri.ends_with(":collection")) {
    path = "/me/tracks?limit=50";
  } else {
    return nullptr;
  }
  try {
    return track_list(json::parse(api("GET", path).body).value("items", json::array()));
  } catch (const NotSignedIn &) {
    return nullptr;
  } catch (const std::exception &e) {
    log_warning("spotify: reading the tracks of {} failed: {}", uri, e.what());
    return nullptr;
  }
}

Spotify::Result Spotify::transfer(const std::string &device_id) {
  return act("transfer", "PUT", "/me/player", json{{"device_ids", {device_id}}, {"play", true}}.dump());
}

Spotify::Result Spotify::volume(int percent) {
  return act("volume", "PUT", std::format("/me/player/volume?volume_percent={}", std::clamp(percent, 0, 100)));
}

json Spotify::devices() {
  try {
    const json answer = json::parse(api("GET", "/me/player/devices").body);
    json out = json::array();
    for (const json &d : answer.value("devices", json::array())) {
      if (d.value("is_restricted", false)) continue; // cannot be controlled through the API
      out.push_back({{"id", text(d, "id")},
                     {"name", text(d, "name")},
                     {"type", text(d, "type")},
                     {"isActive", d.value("is_active", false)},
                     {"volume", d.value("supports_volume", true) && d.contains("volume_percent") &&
                                        d["volume_percent"].is_number()
                                    ? d["volume_percent"]
                                    : json(nullptr)}});
    }
    return out;
  } catch (const NotSignedIn &) {
    return nullptr;
  } catch (const std::exception &e) {
    log_warning("spotify: listing devices failed: {}", e.what());
    return nullptr;
  }
}

void Spotify::refresh_library() {
  try {
    const json playlists = json::parse(api("GET", "/me/playlists?limit=24").body);
    const json recent = json::parse(api("GET", "/me/player/recently-played?limit=50").body);
    const json me = json::parse(api("GET", "/me").body);
    hub_.publish("musicLibrary", music_library(playlists, recent, text(me, "id")));
  } catch (const NotSignedIn &) {
    hub_.publish("musicLibrary", nullptr);
  } catch (const FetchError &e) {
    if (e.status == 401 || e.status == 403) {
      log_warning("spotify: no playlists, the login lacks the scopes for them; sign in again with "
                  "tools/spotify-login.py");
      hub_.publish("musicLibrary", nullptr);
      return;
    }
    throw;
  }
}

void Spotify::run(std::stop_token stop) {
  json shown = nullptr;
  hub_.publish("music", shown);
  hub_.publish("musicLibrary", nullptr);
  constexpr auto kLibraryEvery = 30min;
  auto library_due = std::chrono::steady_clock::now();
  // At boot the network comes up about a second after the server: retry soon at first.
  constexpr std::chrono::milliseconds kFirstRetry = 5s;
  std::chrono::milliseconds retry = kFirstRetry;
  bool failing = false; // log the recovery after a failure

  while (!stop.stop_requested()) {
    std::chrono::milliseconds next = kIdlePoll;
    try {
      const std::string token = access_token();
      if (token.empty()) {
        next = kNoLoginPoll;
        if (!shown.is_null()) hub_.publish("music", shown = nullptr);
      } else {
        const auto response = http_fetch({.url = std::string(kApi) + "/me/player?additional_types=episode",
                                          .headers = {"Authorization: Bearer " + token},
                                          .method = "GET",
                                          .body = {},
                                          .content_type = {},
                                          .timeout = 10s});
        // 204: no device is playing or paused at the moment.
        const json playing = response.status == 204 || response.body.empty()
                                 ? json(nullptr)
                                 : now_playing(json::parse(response.body), now_ms());
        if (worth_publishing(shown, playing)) hub_.publish("music", shown = playing);
        if (library_stale_.exchange(false)) library_due = std::chrono::steady_clock::now();
        if (std::chrono::steady_clock::now() >= library_due) {
          refresh_library();
          library_due = std::chrono::steady_clock::now() + kLibraryEvery;
        }
        if (failing) log_info("spotify: reading the player again");
        failing = false;
        if (playing.is_object() && playing["isPlaying"].get<bool>()) {
          // Also look right after the track should end, to catch the next one at once.
          const auto left = std::chrono::milliseconds(integer(playing, "durationMs") - integer(playing, "progressMs"));
          next = std::clamp<std::chrono::milliseconds>(left + 1s, 1s, kPlayingPoll);
        }
        retry = kFirstRetry;
      }
    } catch (const FetchError &e) {
      if (e.status == 401) {
        const std::lock_guard lock(token_mutex_);
        access_token_.clear();
        next = 1s;
      } else if (e.status == 429) {
        next = std::chrono::seconds(e.retry_after_s.value_or(30));
        log_warning("spotify: rate limited, waiting {} s", std::chrono::duration_cast<std::chrono::seconds>(next).count());
      } else {
        next = retry;
        retry = std::min<std::chrono::milliseconds>(retry * 2, 5min);
        log_warning("spotify: reading the player failed, retrying in {} s: {}",
                    std::chrono::duration_cast<std::chrono::seconds>(next).count(), e.what());
        failing = true;
      }
    } catch (const std::exception &e) {
      next = retry;
      log_warning("spotify: {}", e.what());
    }

    std::unique_lock lock(wake_mutex_);
    if (wake_.wait_for(lock, stop, next, [this] { return poke_; })) {
      poke_ = false;
      lock.unlock();
      // A command just ran: give Spotify a moment, then read again.
      std::mutex pause_mutex;
      std::unique_lock pause(pause_mutex);
      std::condition_variable_any().wait_for(pause, stop, kAfterCommand, [] { return false; });
    }
  }
}

} // namespace dash
