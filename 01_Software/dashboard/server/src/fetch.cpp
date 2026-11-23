#include "fetch.hpp"

#include <curl/curl.h>

#include <format>
#include <memory>

namespace dash {

namespace {

constexpr const char *kUserAgent = "out-of-fun-dashboard/0.1";

std::size_t append(char *data, std::size_t size, std::size_t count, void *out) {
  static_cast<std::string *>(out)->append(data, size * count);
  return size * count;
}

struct CurlDeleter {
  void operator()(CURL *curl) const { curl_easy_cleanup(curl); }
};

} // namespace

std::string http_get(const std::string &url, std::chrono::seconds timeout) {
  const std::unique_ptr<CURL, CurlDeleter> curl(curl_easy_init());
  if (!curl) throw FetchError("curl_easy_init failed");

  std::string body;
  char error[CURL_ERROR_SIZE] = {};
  curl_easy_setopt(curl.get(), CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl.get(), CURLOPT_USERAGENT, kUserAgent);
  curl_easy_setopt(curl.get(), CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT, static_cast<long>(timeout.count()));
  curl_easy_setopt(curl.get(), CURLOPT_CONNECTTIMEOUT, 10L);
  curl_easy_setopt(curl.get(), CURLOPT_NOSIGNAL, 1L); // called from worker threads
  curl_easy_setopt(curl.get(), CURLOPT_ACCEPT_ENCODING, "");
  curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, append);
  curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &body);
  curl_easy_setopt(curl.get(), CURLOPT_ERRORBUFFER, error);

  const CURLcode rc = curl_easy_perform(curl.get());
  if (rc != CURLE_OK) {
    throw FetchError(std::format("{}: {}", url, error[0] ? error : curl_easy_strerror(rc)));
  }
  long status = 0;
  curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &status);
  if (status < 200 || status >= 300) {
    throw FetchError(std::format("{}: HTTP {}: {}", url, status, body.substr(0, 200)));
  }
  return body;
}

std::string url_encode(std::string_view text) {
  std::string out;
  for (const unsigned char c : text) {
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_' ||
        c == '.' || c == '~') {
      out += static_cast<char>(c);
    } else {
      out += std::format("%{:02X}", c);
    }
  }
  return out;
}

} // namespace dash
