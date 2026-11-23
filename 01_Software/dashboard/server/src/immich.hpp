// Photos from Immich (stage D3), for the photos page, the summary and the screensaver.
// Every 30 min a new random batch of landscape photos from the whole library, plus today's
// memory ("Rok temu", "3 lata temu"). The previews are downloaded to <data dir>/photos and
// served as GET /api/photos/<file>, so the API key never reaches the page and the
// screensaver never waits for Immich. Field names follow Photos in web/src/lib/types.ts.
#pragma once

#include <filesystem>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <thread>

#include "config.hpp"
#include "hub.hpp"

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
  PhotosProvider(Hub &hub, ImmichSettings settings, std::filesystem::path cache_dir);

private:
  std::jthread thread_;
};

} // namespace dash
