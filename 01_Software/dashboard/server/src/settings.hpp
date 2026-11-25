// Settings changed on the screen (the settings sheet), kept in <data dir>/settings.json so they
// survive restarts and updates, and published as "settings" (Settings of web/src/lib/types.ts):
//   photoIntervalS   seconds each photo stays, 5-600 (default 20)
//   photoOrder       "random" (a random batch from the whole library) or "newest" (the library
//                    from the newest photo back, batch after batch)
//   saverAfterS      seconds without a touch before the screensaver, 30-3600, or 0 for never
//                    (default 300)
//   nightEnabled     the night screensaver: black, only the clock and the date
//   nightFrom/To     its hours, "HH:MM"; may run past midnight
//   hiddenPlaylists  spotify: URIs left out of the music page's library
// Unlike config.json, which is set up once by hand, these are the user's choices.
#pragma once

#include <filesystem>
#include <functional>
#include <mutex>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "hub.hpp"

namespace dash {

/** The defaults, merged with what the file has. */
nlohmann::json default_settings();

/**
 * Checks a change from the page against the fields above. Returns the error, or an empty
 * string when the whole change is valid (nothing is applied otherwise).
 */
std::string check_settings(const nlohmann::json &change);

class SettingsStore {
public:
  SettingsStore(Hub &hub, std::filesystem::path file);

  nlohmann::json get() const;

  /** Applies a valid change (see check_settings), saves and publishes it; else the error. */
  std::string apply(const nlohmann::json &change);

  /** Called after every applied change, with the new settings (from the HTTP thread). */
  void on_change(std::function<void(const nlohmann::json &)> listener);

private:
  void save() const;

  Hub &hub_;
  std::filesystem::path file_;
  mutable std::mutex mutex_;
  nlohmann::json settings_;
  std::vector<std::function<void(const nlohmann::json &)>> listeners_;
};

} // namespace dash
