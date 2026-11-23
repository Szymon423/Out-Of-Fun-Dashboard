#include "log.hpp"

#include <cstdio>
#include <mutex>
#include <string>

namespace dash {

void log_line(Level level, std::string_view text) {
  static std::mutex mutex;
  const std::string line = std::format("<{}>{}\n", static_cast<int>(level), text);
  const std::lock_guard lock(mutex);
  std::fputs(line.c_str(), stderr);
}

}  // namespace dash
