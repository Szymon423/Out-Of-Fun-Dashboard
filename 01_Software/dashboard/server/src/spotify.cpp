#include "spotify.hpp"

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
      {"artist", artist},
      {"album", album},
      {"artUrl", cover(images)},
      {"isPlaying", player.value("is_playing", false)},
      {"progressMs", integer(player, "progress_ms")},
      {"readAt", now},
      {"durationMs", integer(*item, "duration_ms")},
      {"device", player.contains("device") ? text(player["device"], "name") : std::string()},
  };
}

bool worth_publishing(const json &shown, const json &next) {
  if (shown.is_null() || next.is_null()) return shown.is_null() != next.is_null();
  for (const char *key : {"title", "artist", "album", "artUrl", "isPlaying", "durationMs", "device"}) {
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
}

std::string Spotify::access_token() {
  const std::lock_guard lock(token_mutex_);
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

Spotify::Result Spotify::command(std::string_view action) {
  struct Call {
    std::string_view action;
    const char *method;
    const char *path;
  };
  static constexpr Call kCalls[] = {
      {"play", "PUT", "/me/player/play"},
      {"pause", "PUT", "/me/player/pause"},
      {"next", "POST", "/me/player/next"},
      {"previous", "POST", "/me/player/previous"},
  };
  const Call *call = nullptr;
  for (const Call &c : kCalls) {
    if (c.action == action) call = &c;
  }
  if (!call) return Result::failed;

  for (int attempt = 0; attempt < 2; ++attempt) {
    try {
      const std::string token = access_token();
      if (token.empty()) return Result::not_signed_in;
      http_fetch({.url = std::string(kApi) + call->path,
                  .headers = {"Authorization: Bearer " + token},
                  .method = call->method,
                  .body = {},
                  .content_type = {},
                  .timeout = 10s});
      {
        const std::lock_guard lock(wake_mutex_);
        poke_ = true;
      }
      wake_.notify_all();
      return Result::ok;
    } catch (const FetchError &e) {
      if (e.status == 401 && attempt == 0) { // the access token ran out early: renew and retry
        const std::lock_guard lock(token_mutex_);
        access_token_.clear();
        continue;
      }
      if (e.status == 404) return Result::no_device; // NO_ACTIVE_DEVICE
      log_warning("spotify: {} failed: {}", action, e.what());
      return Result::failed;
    } catch (const std::exception &e) {
      log_warning("spotify: {} failed: {}", action, e.what());
      return Result::failed;
    }
  }
  return Result::failed;
}

void Spotify::run(std::stop_token stop) {
  json shown = nullptr;
  hub_.publish("music", shown);
  std::chrono::milliseconds retry = 30s;

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
        if (playing.is_object() && playing["isPlaying"].get<bool>()) {
          // Also look right after the track should end, to catch the next one at once.
          const auto left = std::chrono::milliseconds(integer(playing, "durationMs") - integer(playing, "progressMs"));
          next = std::clamp<std::chrono::milliseconds>(left + 1s, 1s, kPlayingPoll);
        }
        retry = 30s;
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
        log_warning("spotify: reading the player failed: {}", e.what());
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
