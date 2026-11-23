#include "systemd.hpp"

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <cstdlib>
#include <cstring>
#include <string>

#include "log.hpp"

namespace dash {

void sd_notify(std::string_view state) {
  const char *path = std::getenv("NOTIFY_SOCKET");
  if (!path || !*path) return;

  sockaddr_un addr{};
  addr.sun_family = AF_UNIX;
  const std::size_t len = std::strlen(path);
  if (len >= sizeof addr.sun_path) return;
  std::memcpy(addr.sun_path, path, len);
  if (addr.sun_path[0] == '@') addr.sun_path[0] = '\0'; // abstract socket
  const auto addr_len = static_cast<socklen_t>(offsetof(sockaddr_un, sun_path) + len);

  const int fd = socket(AF_UNIX, SOCK_DGRAM | SOCK_CLOEXEC, 0);
  if (fd < 0) return;
  if (sendto(fd, state.data(), state.size(), 0, reinterpret_cast<const sockaddr *>(&addr), addr_len) < 0) {
    log_warning("sd_notify({}) failed: {}", state, std::strerror(errno));
  }
  close(fd);
}

} // namespace dash
