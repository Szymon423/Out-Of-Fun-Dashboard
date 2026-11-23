// Log lines to stderr, which systemd puts into the journal. The prefix <N> is the syslog
// priority, so `journalctl -p warning` finds the problems.
#pragma once

#include <format>
#include <string_view>

namespace dash {

enum class Level { error = 3, warning = 4, info = 6 };

void log_line(Level level, std::string_view text);

template <typename... Args>
void log_info(std::format_string<Args...> fmt, Args &&...args) {
  log_line(Level::info, std::format(fmt, std::forward<Args>(args)...));
}

template <typename... Args>
void log_warning(std::format_string<Args...> fmt, Args &&...args) {
  log_line(Level::warning, std::format(fmt, std::forward<Args>(args)...));
}

template <typename... Args>
void log_error(std::format_string<Args...> fmt, Args &&...args) {
  log_line(Level::error, std::format(fmt, std::forward<Args>(args)...));
}

}  // namespace dash
