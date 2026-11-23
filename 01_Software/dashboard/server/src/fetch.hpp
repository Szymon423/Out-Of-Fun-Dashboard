// HTTPS requests to the services on the internet, with libcurl.
#pragma once

#include <chrono>
#include <stdexcept>
#include <string>
#include <string_view>

namespace dash {

struct FetchError : std::runtime_error {
  using std::runtime_error::runtime_error;
};

/** GET the URL and return the body; throws FetchError on any failure or a non-2xx status. */
std::string http_get(const std::string &url, std::chrono::seconds timeout = std::chrono::seconds(20));

/** Percent-encodes a query parameter value. */
std::string url_encode(std::string_view text);

} // namespace dash
