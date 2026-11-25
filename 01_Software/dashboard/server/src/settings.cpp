#include "settings.hpp"

#include <fstream>
#include <regex>

#include "log.hpp"

namespace dash {

using nlohmann::json;

json default_settings() {
  return {
      {"photoIntervalS", 20},     {"photoOrder", "random"}, {"nightEnabled", false},
      {"nightFrom", "22:00"},     {"nightTo", "06:30"},     {"hiddenPlaylists", json::array()},
  };
}

std::string check_settings(const json &change) {
  static const std::regex kTime("^([01][0-9]|2[0-3]):[0-5][0-9]$");
  if (!change.is_object() || change.empty()) return "expected a JSON object with settings";
  for (const auto &[key, value] : change.items()) {
    if (key == "photoIntervalS") {
      if (!value.is_number_integer() || value.get<int>() < 5 || value.get<int>() > 600) {
        return "photoIntervalS: a whole number of seconds, 5-600";
      }
    } else if (key == "photoOrder") {
      if (value != "random" && value != "newest") return R"(photoOrder: "random" or "newest")";
    } else if (key == "nightEnabled") {
      if (!value.is_boolean()) return "nightEnabled: true or false";
    } else if (key == "nightFrom" || key == "nightTo") {
      if (!value.is_string() || !std::regex_match(value.get<std::string>(), kTime)) return key + R"(: "HH:MM")";
    } else if (key == "hiddenPlaylists") {
      if (!value.is_array() || value.size() > 200) return "hiddenPlaylists: a list of spotify: URIs";
      for (const json &uri : value) {
        if (!uri.is_string() || !uri.get<std::string>().starts_with("spotify:")) {
          return "hiddenPlaylists: a list of spotify: URIs";
        }
      }
    } else {
      return "unknown setting " + key;
    }
  }
  return {};
}

SettingsStore::SettingsStore(Hub &hub, std::filesystem::path file)
    : hub_(hub), file_(std::move(file)), settings_(default_settings()) {
  try {
    if (std::filesystem::exists(file_)) {
      const json saved = json::parse(std::ifstream(file_));
      // Take the saved fields one by one, so one bad field does not lose the others.
      for (const auto &[key, value] : saved.items()) {
        if (check_settings(json{{key, value}}).empty()) settings_[key] = value;
      }
    }
  } catch (const std::exception &e) {
    log_warning("settings: ignoring {}: {}", file_.string(), e.what());
  }
  hub_.publish("settings", settings_);
}

json SettingsStore::get() const {
  const std::lock_guard lock(mutex_);
  return settings_;
}

std::string SettingsStore::apply(const json &change) {
  if (std::string error = check_settings(change); !error.empty()) return error;
  json now;
  std::vector<std::function<void(const json &)>> listeners;
  {
    const std::lock_guard lock(mutex_);
    settings_.merge_patch(change);
    save();
    now = settings_;
    listeners = listeners_;
  }
  hub_.publish("settings", now);
  for (const auto &listener : listeners) listener(now);
  return {};
}

void SettingsStore::on_change(std::function<void(const json &)> listener) {
  const std::lock_guard lock(mutex_);
  listeners_.push_back(std::move(listener));
}

void SettingsStore::save() const {
  // Called with mutex_ held.
  const auto tmp = std::filesystem::path(file_).concat(".tmp");
  {
    std::ofstream out(tmp);
    out << settings_.dump(2) << '\n';
    if (!out) {
      log_warning("settings: cannot write {}", tmp.string());
      return;
    }
  }
  std::error_code ec;
  std::filesystem::rename(tmp, file_, ec);
  if (ec) log_warning("settings: cannot save {}: {}", file_.string(), ec.message());
}

} // namespace dash
