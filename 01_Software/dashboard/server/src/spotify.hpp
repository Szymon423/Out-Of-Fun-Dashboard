// Spotify (stage D4): what plays where, and play, pause, next and previous.
//
// Signing in happens once, on the PC: tools/spotify-login.py (authorization code with PKCE,
// no client secret) leaves a refresh token in <data dir>/spotify.json. The server renews its
// access token from it and keeps the newest refresh token there, as Spotify may hand out a
// new one on every renewal. Without the file it publishes no music and looks for the file
// again every 30 s, so a login takes effect without a restart.
//
// The player is read every 5 s while music plays and every 20 s otherwise. "music" is only
// published when something changes that the page cannot work out itself: another track, play
// or pause, another device, or a jump in the position (the page moves the progress bar on by
// itself from progressMs and readAt).
#pragma once

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <filesystem>
#include <mutex>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <thread>

#include "hub.hpp"

namespace dash {

/** NowPlaying of types.ts from GET /v1/me/player, read at `now_ms`; null when nothing plays. */
nlohmann::json now_playing(const nlohmann::json &player, std::int64_t now_ms);

/** True when `next` says more than the page already knows from `shown` (see the top). */
bool worth_publishing(const nlohmann::json &shown, const nlohmann::json &next);

class Spotify {
public:
  Spotify(Hub &hub, std::string client_id, std::filesystem::path token_file);
  ~Spotify();

  enum class Result { ok, not_signed_in, no_device, failed };

  /** play, pause, next or previous; called from the HTTP threads. */
  Result command(std::string_view action);

private:
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

  std::mutex wake_mutex_;
  std::condition_variable_any wake_;
  bool poke_ = false; // a command ran: read the player again soon

  std::jthread thread_; // last: it starts with everything above ready
};

} // namespace dash
