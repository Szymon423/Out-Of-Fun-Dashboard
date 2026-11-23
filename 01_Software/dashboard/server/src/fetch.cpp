#include "fetch.hpp"

#include <curl/curl.h>

#include <algorithm>
#include <cctype>
#include <format>
#include <memory>

namespace dash {

namespace {

constexpr const char *kUserAgent = "out-of-fun-dashboard/0.1";

std::size_t append(char *data, std::size_t size, std::size_t count, void *out) {
  static_cast<std::string *>(out)->append(data, size * count);
  return size * count;
}

/** Keeps the Retry-After header, the only response header we need. */
std::size_t header(char *data, std::size_t size, std::size_t count, void *out) {
  const std::string_view line(data, size * count);
  constexpr std::string_view kName = "retry-after:";
  if (line.size() > kName.size()) {
    std::string name(line.substr(0, kName.size()));
    std::ranges::transform(name, name.begin(), [](unsigned char c) { return std::tolower(c); });
    if (name == kName) {
      try {
        *static_cast<std::optional<int> *>(out) = std::stoi(std::string(line.substr(kName.size())));
      } catch (const std::exception &) {
        // an HTTP date instead of seconds: ignore, the caller has its own backoff
      }
    }
  }
  return size * count;
}

struct CurlDeleter {
  void operator()(CURL *curl) const { curl_easy_cleanup(curl); }
};

struct SlistDeleter {
  void operator()(curl_slist *list) const { curl_slist_free_all(list); }
};

} // namespace

HttpResponse http_fetch(const HttpRequest &request) {
  const std::string &url = request.url;
  const std::unique_ptr<CURL, CurlDeleter> curl(curl_easy_init());
  if (!curl) throw FetchError("curl_easy_init failed");

  const bool sends_body = request.method == "POST" || request.method == "PUT";
  std::vector<std::string> lines = request.headers;
  if (sends_body && !request.content_type.empty()) lines.push_back("Content-Type: " + request.content_type);
  curl_slist *list = nullptr;
  for (const auto &line : lines) {
    curl_slist *longer = curl_slist_append(list, line.c_str());
    if (!longer) {
      curl_slist_free_all(list);
      throw FetchError("curl_slist_append failed");
    }
    list = longer;
  }
  const std::unique_ptr<curl_slist, SlistDeleter> headers(list);

  if (sends_body) {
    curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS, request.body.c_str());
    curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDSIZE, static_cast<long>(request.body.size()));
    if (request.method == "PUT") curl_easy_setopt(curl.get(), CURLOPT_CUSTOMREQUEST, "PUT");
  } else if (request.method != "GET") {
    throw FetchError("unsupported method " + request.method);
  }
  if (headers) curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, headers.get());

  std::string body;
  std::optional<int> retry_after;
  char error[CURL_ERROR_SIZE] = {};
  curl_easy_setopt(curl.get(), CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl.get(), CURLOPT_USERAGENT, kUserAgent);
  curl_easy_setopt(curl.get(), CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT, static_cast<long>(request.timeout.count()));
  curl_easy_setopt(curl.get(), CURLOPT_CONNECTTIMEOUT, 10L);
  curl_easy_setopt(curl.get(), CURLOPT_NOSIGNAL, 1L); // called from worker threads
  curl_easy_setopt(curl.get(), CURLOPT_ACCEPT_ENCODING, "");
  curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, append);
  curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &body);
  curl_easy_setopt(curl.get(), CURLOPT_HEADERFUNCTION, header);
  curl_easy_setopt(curl.get(), CURLOPT_HEADERDATA, &retry_after);
  curl_easy_setopt(curl.get(), CURLOPT_ERRORBUFFER, error);

  const CURLcode rc = curl_easy_perform(curl.get());
  if (rc != CURLE_OK) {
    throw FetchError(std::format("{}: {}", url, error[0] ? error : curl_easy_strerror(rc)));
  }
  long status = 0;
  curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &status);
  if (status < 200 || status >= 300) {
    throw FetchError(std::format("{}: HTTP {}: {}", url, status, body.substr(0, 200)), status, retry_after);
  }
  const char *type = nullptr;
  curl_easy_getinfo(curl.get(), CURLINFO_CONTENT_TYPE, &type);
  return {status, std::move(body), type ? type : ""};
}

std::string http_get(const std::string &url, std::chrono::seconds timeout) {
  return http_fetch({.url = url, .headers = {}, .method = "GET", .body = {}, .content_type = {}, .timeout = timeout})
      .body;
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

std::string form_body(const std::map<std::string, std::string> &fields) {
  std::string out;
  for (const auto &[name, value] : fields) {
    if (!out.empty()) out += '&';
    out += url_encode(name) + '=' + url_encode(value);
  }
  return out;
}

} // namespace dash
