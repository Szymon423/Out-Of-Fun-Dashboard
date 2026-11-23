// HTTP(S) requests to the services the dashboard reads, with libcurl.
#pragma once

#include <chrono>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace dash {

struct FetchError : std::runtime_error {
  explicit FetchError(const std::string &what, long status = 0) : std::runtime_error(what), status(status) {}
  long status; // the HTTP status of a refused request; 0 when there was no answer
};

struct HttpRequest {
  std::string url;
  std::vector<std::string> headers;     // "Name: value"
  std::optional<std::string> post_json; // POST with this JSON body; GET without it
  std::chrono::seconds timeout{20};
};

struct HttpResponse {
  std::string body;
  std::string content_type;
};

/** Sends the request; throws FetchError on any failure or a non-2xx status. */
HttpResponse http_fetch(const HttpRequest &request);

/** GET the URL and return the body; throws FetchError like http_fetch. */
std::string http_get(const std::string &url, std::chrono::seconds timeout = std::chrono::seconds(20));

/** Percent-encodes a query parameter value. */
std::string url_encode(std::string_view text);

} // namespace dash
