// Tells systemd the service is ready (Type=notify), without linking libsystemd: the
// protocol is one datagram to the socket in $NOTIFY_SOCKET. Does nothing outside systemd.
#pragma once

#include <string_view>

namespace dash {

void sd_notify(std::string_view state);

} // namespace dash
