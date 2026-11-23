// The screen's backlight, through /sys/class/backlight/<device>/brightness. The frontend
// speaks in percent; the Touch Display 2 has 32 steps (0-31). Without a backlight (a PC, or
// --backlight none) the value is only kept, so the slider still works.
#pragma once

#include <filesystem>
#include <mutex>
#include <optional>

#include "hub.hpp"

namespace dash {

class Display {
public:
  /** `backlight` is the sysfs directory of the device, or nothing for none. */
  Display(Hub &hub, std::optional<std::filesystem::path> backlight);

  /** Sets the brightness, 0-100 %. False when the backlight could not be written. */
  bool set_brightness(int percent);

  /** The first device in /sys/class/backlight, if there is one. */
  static std::optional<std::filesystem::path> find_backlight();

private:
  Hub &hub_;
  std::optional<std::filesystem::path> backlight_;
  int max_ = 0; // max_brightness of the device
  std::mutex mutex_;
};

} // namespace dash
