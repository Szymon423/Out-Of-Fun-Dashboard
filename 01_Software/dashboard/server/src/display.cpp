#include "display.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <nlohmann/json.hpp>

#include "log.hpp"

namespace dash {

namespace {

constexpr int kDefaultPercent = 80; // shown when there is no backlight to read

std::optional<int> read_int(const std::filesystem::path &file) {
  int value = 0;
  if (std::ifstream in(file); in >> value) return value;
  return std::nullopt;
}

} // namespace

std::optional<std::filesystem::path> Display::find_backlight() {
  std::error_code ec;
  for (const auto &entry : std::filesystem::directory_iterator("/sys/class/backlight", ec)) return entry.path();
  return std::nullopt;
}

Display::Display(Hub &hub, std::optional<std::filesystem::path> backlight) : hub_(hub), backlight_(std::move(backlight)) {
  int percent = kDefaultPercent;
  if (backlight_) {
    const auto max = read_int(*backlight_ / "max_brightness");
    const auto now = read_int(*backlight_ / "brightness");
    if (max && *max > 0 && now) {
      max_ = *max;
      percent = static_cast<int>(std::lround(*now * 100.0 / max_));
      log_info("display: backlight {} at {}/{} ({} %)", backlight_->string(), *now, max_, percent);
    } else {
      log_warning("display: cannot read {}, keeping brightness in memory only", backlight_->string());
      backlight_.reset();
    }
  }
  hub_.publish("display", {{"brightness", percent}});
}

bool Display::set_brightness(int percent) {
  percent = std::clamp(percent, 0, 100);
  const std::lock_guard lock(mutex_);
  bool ok = true;
  if (backlight_) {
    // Never 0: on this panel that switches the backlight off, which looks like a dead screen.
    const int raw = std::max(1, static_cast<int>(std::lround(percent * max_ / 100.0)));
    std::ofstream out(*backlight_ / "brightness");
    out << raw << std::flush;
    if (!out) {
      log_warning("display: cannot write {}/brightness (permissions?)", backlight_->string());
      ok = false;
    }
  }
  hub_.publish("display", {{"brightness", percent}});
  return ok;
}

} // namespace dash
