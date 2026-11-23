#include "immich.hpp"

#include <algorithm>
#include <condition_variable>
#include <ctime>
#include <fstream>
#include <mutex>
#include <regex>
#include <set>

#include "fetch.hpp"
#include "log.hpp"

namespace dash {

using nlohmann::json;
using namespace std::chrono_literals;
namespace fs = std::filesystem;

namespace {

constexpr int kAsk = 100;     // random assets asked for; portraits and the like drop out
constexpr int kBatch = 30;    // photos in one batch: 10 min of screensaver at 20 s each
constexpr auto kRefresh = 30min;
constexpr auto kFirstRetry = 1min;
constexpr const char *kAlbum = "Wszystkie zdjęcia";

std::string text(const json &object, const char *key) {
  const auto it = object.find(key);
  return it != object.end() && it->is_string() ? it->get<std::string>() : std::string();
}

double number(const json &object, const char *key) {
  const auto it = object.find(key);
  return it != object.end() && it->is_number() ? it->get<double>() : 0.0;
}

/** The EXIF orientation as a number; Immich sends it as a string like "6". */
int orientation(const json &exif) {
  const auto it = exif.find("orientation");
  if (it == exif.end()) return 1;
  if (it->is_number()) return it->get<int>();
  if (it->is_string()) {
    try {
      return std::stoi(it->get<std::string>());
    } catch (const std::exception &) {
    }
  }
  return 1;
}

std::string local_date() {
  const std::time_t t = std::time(nullptr);
  std::tm tm{};
  localtime_r(&t, &tm);
  char buf[16];
  std::strftime(buf, sizeof buf, "%Y-%m-%d", &tm);
  return buf;
}

int local_year() {
  const std::time_t t = std::time(nullptr);
  std::tm tm{};
  localtime_r(&t, &tm);
  return tm.tm_year + 1900;
}

bool sleep_for(std::stop_token stop, std::chrono::seconds duration) {
  std::mutex mutex;
  std::condition_variable_any cv;
  std::unique_lock lock(mutex);
  cv.wait_for(lock, stop, duration, [] { return false; });
  return !stop.stop_requested();
}

class Immich {
public:
  Immich(ImmichSettings settings, fs::path cache) : settings_(std::move(settings)), cache_(std::move(cache)) {}

  json random_assets() const {
    return json::parse(call("/api/search/random", json{{"size", kAsk}, {"type", "IMAGE"}, {"withExif", true}}.dump()));
  }

  json memories() const { return json::parse(call("/api/memories?for=" + local_date() + "T00:00:00.000Z")); }

  /** Downloads the preview unless it is cached; returns the file name. */
  std::string preview(const std::string &id) const {
    static const std::regex kUuid("^[0-9a-f-]{36}$"); // the id becomes a file name
    if (!std::regex_match(id, kUuid)) throw std::runtime_error("unexpected asset id " + id);
    for (const char *ext : {".jpg", ".webp"}) {
      if (fs::exists(cache_ / (id + ext))) return id + ext;
    }
    const auto response = fetch("/api/assets/" + id + "/thumbnail?size=preview", std::nullopt, 30s);
    const auto file = preview_file(id, response.content_type);
    if (!file) throw FetchError(std::format("preview of {}: unexpected type {}", id, response.content_type));
    const fs::path tmp = cache_ / (*file + ".tmp");
    {
      std::ofstream out(tmp, std::ios::binary);
      out.write(response.body.data(), static_cast<std::streamsize>(response.body.size()));
      if (!out) throw std::runtime_error("cannot write " + tmp.string());
    }
    fs::rename(tmp, cache_ / *file);
    return *file;
  }

  /** Deletes cached previews that are not in `keep`. */
  void prune(const std::set<std::string> &keep) const {
    std::error_code ec;
    for (const auto &entry : fs::directory_iterator(cache_, ec)) {
      if (!keep.contains(entry.path().filename().string())) fs::remove(entry.path(), ec);
    }
  }

private:
  HttpResponse fetch(const std::string &path, std::optional<std::string> post_json,
                     std::chrono::seconds timeout = 20s) const {
    return http_fetch({.url = settings_.url + path,
                       .headers = {"x-api-key: " + settings_.api_key, "Accept: application/json, image/*"},
                       .post_json = std::move(post_json),
                       .timeout = timeout});
  }

  std::string call(const std::string &path, std::optional<std::string> post_json = std::nullopt) const {
    return fetch(path, std::move(post_json)).body;
  }

  ImmichSettings settings_;
  fs::path cache_;
};

/** A new batch: random assets that fit the screen, with their previews cached. A photo
 *  whose preview fails is left out; a missing permission fails the whole batch. */
json fetch_batch(const Immich &immich) {
  json items = json::array();
  for (const json &asset : immich.random_assets()) {
    if (items.size() >= kBatch) break;
    if (!fits_screen(asset)) continue;
    try {
      items.push_back(photo_json(asset, immich.preview(asset.at("id").get<std::string>())));
    } catch (const FetchError &e) {
      if (e.status == 403) throw;
      log_warning("photos: skipping one photo: {}", e.what());
    }
  }
  return items;
}

/** Today's memory from Immich's memories, the most recent year first; null when none. */
json pick_memory(const Immich &immich, const json &memories) {
  json best = nullptr;
  int best_year = 0;
  for (const json &memory : memories) {
    const int year = memory.contains("data") ? memory["data"].value("year", 0) : 0;
    const json &assets = memory.contains("assets") ? memory["assets"] : json::array();
    if (year <= best_year || year >= local_year() || assets.empty()) continue;
    // Prefer a picture that fits the screen; any photo will do for the small widget.
    auto pick = std::find_if(assets.begin(), assets.end(), fits_screen);
    if (pick == assets.end()) {
      pick = std::find_if(assets.begin(), assets.end(), [](const json &a) { return text(a, "type") == "IMAGE"; });
    }
    if (pick == assets.end()) continue;
    best = {{"title", years_ago(local_year() - year)},
            {"photo", photo_json(*pick, immich.preview(pick->at("id").get<std::string>()))}};
    best_year = year;
  }
  return best;
}

void file_names(const json &photos, std::set<std::string> &out) {
  for (const json &item : photos) {
    const std::string url = text(item, "url");
    out.insert(url.substr(url.rfind('/') + 1));
  }
}

void run(std::stop_token stop, Hub &hub, const Immich &immich) {
  std::chrono::seconds retry = kFirstRetry;
  json previous = json::array(); // the last batch; the page may still show it
  json memory = nullptr;
  std::string memory_date;
  bool failing = true;

  while (!stop.stop_requested()) {
    std::chrono::seconds next = kRefresh;
    try {
      if (memory_date != local_date()) {
        json memories = json::array();
        try {
          memories = immich.memories();
        } catch (const FetchError &e) {
          if (e.status != 403) throw;
          log_warning("photos: no memories today, the API key lacks the memory.read permission");
        }
        memory = pick_memory(immich, memories); // a failing preview is retried with the batch
        memory_date = local_date();             // once a day, or after a restart
      }
      json items = fetch_batch(immich);
      if (items.empty()) {
        log_warning("photos: no landscape photos in this random batch, trying again soon");
        next = 5min;
      } else {
        hub.publish("photos", {{"album", kAlbum}, {"items", items}, {"memory", memory}});
        std::set<std::string> keep;
        file_names(items, keep);
        file_names(previous, keep);
        if (memory.is_object()) file_names(json::array({memory["photo"]}), keep);
        immich.prune(keep);
        if (failing) log_info("photos: {} photos from Immich, a new batch every {} min", items.size(), kRefresh.count());
        previous = std::move(items);
        failing = false;
        retry = kFirstRetry;
      }
    } catch (const std::exception &e) {
      next = retry;
      retry = std::min<std::chrono::seconds>(retry * 2, kRefresh);
      const auto *fetch_error = dynamic_cast<const FetchError *>(&e);
      const bool forbidden = fetch_error && fetch_error->status == 403;
      log_warning("photos: {}, retrying in {} s: {}", forbidden ? "the API key lacks a permission" : "Immich failed",
                  next.count(), e.what());
      failing = true;
    }
    if (!sleep_for(stop, next)) return;
  }
}

} // namespace

bool fits_screen(const json &asset) {
  if (text(asset, "type") != "IMAGE") return false;
  if (asset.value("isTrashed", false) || asset.value("isOffline", false) || asset.value("isArchived", false)) {
    return false;
  }
  if (const std::string visibility = text(asset, "visibility"); !visibility.empty() && visibility != "timeline") {
    return false;
  }
  const auto exif = asset.find("exifInfo");
  if (exif == asset.end() || !exif->is_object()) return false;
  double width = number(*exif, "exifImageWidth");
  double height = number(*exif, "exifImageHeight");
  if (width <= 0 || height <= 0) return false;
  if (const int o = orientation(*exif); o >= 5 && o <= 8) std::swap(width, height); // turned by 90°
  const double ratio = width / height;
  return ratio >= 1.25 && ratio <= 2.4;
}

json photo_json(const json &asset, const std::string &file) {
  const json exif = asset.contains("exifInfo") && asset["exifInfo"].is_object() ? asset["exifInfo"] : json::object();
  std::string place = text(exif, "city");
  if (place.empty()) place = text(exif, "country");
  // Immich writes the local wall-clock time with a "Z" it does not mean; without it the
  // page reads it as local time, which is what the photo's date should show.
  std::string taken = text(asset, "localDateTime");
  if (taken.ends_with('Z')) taken.pop_back();
  return {
      {"id", asset.at("id")},
      {"url", "/api/photos/" + file},
      {"takenAt", taken},
      {"place", place.empty() ? json(nullptr) : json(place)},
  };
}

std::string years_ago(int years) {
  if (years == 1) return "Rok temu";
  const int last = years % 10;
  const int last_two = years % 100;
  const bool few = last >= 2 && last <= 4 && !(last_two >= 12 && last_two <= 14);
  return std::format("{} {} temu", years, few ? "lata" : "lat");
}

std::optional<std::string> preview_file(const std::string &asset_id, const std::string &content_type) {
  if (content_type.starts_with("image/jpeg")) return asset_id + ".jpg";
  if (content_type.starts_with("image/webp")) return asset_id + ".webp";
  return std::nullopt;
}

PhotosProvider::PhotosProvider(Hub &hub, ImmichSettings settings, fs::path cache_dir)
    : thread_([&hub, settings = std::move(settings), cache_dir = std::move(cache_dir)](std::stop_token stop) {
        std::error_code ec;
        fs::create_directories(cache_dir, ec);
        if (ec) log_warning("photos: cannot create {}: {}", cache_dir.string(), ec.message());
        run(stop, hub, Immich(settings, cache_dir));
      }) {}

} // namespace dash
