// Spotify (stage D4): what plays where, and play, pause, next and previous.
//
// Signing in happens once, on the PC: tools/spotify-login.py (authorization code with PKCE,
// no client secret) leaves a refresh token in <data dir>/spotify.json. The server renews its
// access token from it and keeps the newest refresh token there, as Spotify may hand out a
// new one on every renewal. Without the file it publishes no music and looks for the file
// again every 30 s, and it notices when the file changes (a new login, e.g. with more scopes),
// so a login always takes effect without a restart.
//
// Besides the player it keeps "musicLibrary" up to date every 30 min: the user's playlists, the
// albums played lately and the liked songs, for the grid on the music page. That needs the
// scopes playlist-read-private, playlist-read-collaborative and user-read-recently-played;
// an older login without them still plays, it just shows no library until signed in again.
//
// The player is read every 5 s while music plays and every 20 s otherwise. "music" is only
// published when something changes that the page cannot work out itself: another track, play
// or pause, another device, or a jump in the position (the page moves the progress bar on by
// itself from progressMs and readAt).
#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <filesystem>
#include <mutex>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <thread>

#include "fetch.hpp"
#include "hub.hpp"

namespace dash {

/** NowPlaying of types.ts from GET /v1/me/player, read at `now_ms`; null when nothing plays. */
nlohmann::json now_playing(const nlohmann::json &player, std::int64_t now_ms);

/** True when `next` says more than the page already knows from `shown` (see the top). */
bool worth_publishing(const nlohmann::json &shown, const nlohmann::json &next);

/**
 * The MusicLibrary of types.ts from GET /me/playlists, GET /me/player/recently-played and the
 * user's id: playlists as they come, the albums of the recently played tracks without repeats
 * (at most 8), and the liked songs as a playable collection.
 */
nlohmann::json music_library(const nlohmann::json &playlists, const nlohmann::json &recent, const std::string &user_id);

class Spotify {
public:
  Spotify(Hub &hub, std::string client_id, std::filesystem::path token_file);
  ~Spotify();

  enum class Result { ok, not_signed_in, no_device, failed };

  /** play, pause, next or previous; called from the HTTP threads. */
  Result command(std::string_view action);

  /** Plays a playlist, an album or the liked songs (a spotify: URI), on the given device or,
   *  without one, on the active device. */
  Result play_context(const std::string &uri, const std::string &device_id = {});

  /** Moves the playback to another device, keeping it playing. */
  Result transfer(const std::string &device_id);

  /** Volume of the active device, 0-100. */
  Result volume(int percent);

  /** The user's Spotify Connect devices: [{id, name, type, isActive, volume}]; null on failure. */
  nlohmann::json devices();

private:
  /** One Web API call with a valid token, renewed and retried once on 401. Throws FetchError;
   *  NotSignedIn without a login. */
  struct NotSignedIn {};
  HttpResponse api(const std::string &method, const std::string &path, const std::string &json_body = {});
  Result act(std::string_view what, const std::string &method, const std::string &path, const std::string &body = {});
  void poke();
  void refresh_library();

  /** A valid access token, renewed when needed; empty when not signed in. */
  std::string access_token();
  void load_refresh_token();
  void save_refresh_token(const std::string &token);
  void run(std::stop_token stop);

  Hub &hub_;
  const std::string client_id_;
  const std::filesystem::path token_file_;

  std::mutex token_mutex_;
  std::string refresh_token_;
  std::string access_token_;
  std::chrono::steady_clock::time_point access_expires_;
  std::filesystem::file_time_type token_file_time_{}; // of the file as last read or written
  std::atomic<bool> library_stale_{false};             // a new login: read the library again soon

  std::mutex wake_mutex_;
  std::condition_variable_any wake_;
  bool poke_ = false; // a command ran: read the player again soon

  std::jthread thread_; // last: it starts with everything above ready
};

} // namespace dash
