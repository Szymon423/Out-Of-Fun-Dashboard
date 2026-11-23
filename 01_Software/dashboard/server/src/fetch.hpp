// HTTP(S) requests to the services the dashboard reads, with libcurl.
#pragma once

#include <chrono>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace dash {

struct FetchError : std::runtime_error {
  explicit FetchError(const std::string &what, long status = 0, std::optional<int> retry_after_s = std::nullopt)
      : std::runtime_error(what), status(status), retry_after_s(retry_after_s) {}
  long status;                      // the HTTP status of a refused request; 0 when there was no answer
  std::optional<int> retry_after_s; // Retry-After of a 429 or 503, in seconds
};

struct HttpRequest {
  std::string url;
  std::vector<std::string> headers; // "Name: value"
  std::string method = "GET";       // GET, POST or PUT
  std::string body;                 // sent with POST and PUT, even when empty
  std::string content_type;         // of the body, e.g. application/json
  std::chrono::seconds timeout{20};
};

struct HttpResponse {
  long status = 0; // 2xx; 204 has an empty body
  std::string body;
  std::string content_type;
};

/** Sends the request; throws FetchError on any failure or a non-2xx status. */
HttpResponse http_fetch(const HttpRequest &request);

/** GET the URL and return the body; throws FetchError like http_fetch. */
std::string http_get(const std::string &url, std::chrono::seconds timeout = std::chrono::seconds(20));

/** Percent-encodes a query parameter value. */
std::string url_encode(std::string_view text);

/** An application/x-www-form-urlencoded body. */
std::string form_body(const std::map<std::string, std::string> &fields);

} // namespace dash
