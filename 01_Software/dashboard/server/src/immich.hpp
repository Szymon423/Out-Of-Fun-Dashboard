// Photos from Immich (stage D3), for the photos page, the summary and the screensaver.
// Every 30 min a new batch of landscape photos: random from the whole library, or, with the
// setting photoOrder "newest", the library from the newest photo back. Plus today's
// memory ("Rok temu", "3 lata temu"). The previews are downloaded to <data dir>/photos and
// served as GET /api/photos/<file>, so the API key never reaches the page and the
// screensaver never waits for Immich. Field names follow Photos in web/src/lib/types.ts.
#pragma once

#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <thread>

#include "config.hpp"
#include "hub.hpp"
#include "settings.hpp"

namespace dash {

/**
 * True when the asset is a photo that suits the 1280x720 screen: a landscape picture
 * between 5:4 and 2.4:1 (no portraits, no panoramas), on the timeline (not archived,
 * hidden or locked), neither trashed nor offline. The EXIF orientation is taken into account.
 */
bool fits_screen(const nlohmann::json &asset);

/** A Photo of types.ts for the asset, whose preview is cached as `file`. */
nlohmann::json photo_json(const nlohmann::json &asset, const std::string &file);

/** "Rok temu", "2 lata temu", "5 lat temu", ... */
std::string years_ago(int years);

/** Name of the cached preview of an asset, from the content type Immich sent. */
std::optional<std::string> preview_file(const std::string &asset_id, const std::string &content_type);

/** Publishes "photos" in the background until destroyed. */
class PhotosProvider {
public:
  /** A change of photoOrder in `prefs` starts a new batch at once. */
  PhotosProvider(Hub &hub, ImmichSettings settings, std::filesystem::path cache_dir, SettingsStore &prefs);

  /** Lets the loop sleep until its next batch, or until poked. */
  struct Wake {
    std::mutex mutex;
    std::condition_variable_any cv;
    bool poked = false;
    /** False when stopping. */
    bool sleep(std::stop_token stop, std::chrono::seconds duration);
    void poke();
  };

private:
  std::shared_ptr<Wake> wake_ = std::make_shared<Wake>();
  std::jthread thread_;
};

} // namespace dash
