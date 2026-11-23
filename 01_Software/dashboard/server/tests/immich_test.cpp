// Choosing Immich photos for the screen and turning them into Photos of types.ts. The assets
// are made up, in the shape Immich 2.4 answers /api/search/random with.
#include "check.hpp"
#include "immich.hpp"

using nlohmann::json;

namespace {

json asset(int width, int height, const char *orientation = "1") {
  return {
      {"id", "0b5c2f1e-8d7a-4c3b-9e6f-1a2b3c4d5e6f"},
      {"type", "IMAGE"},
      {"visibility", "timeline"},
      {"isArchived", false},
      {"isTrashed", false},
      {"isOffline", false},
      {"thumbhash", "JfgRDQJod493mIlnh4SYiGhwtAJK"},
      {"localDateTime", "2025-08-14T18:42:10.000Z"},
      {"exifInfo",
       {{"exifImageWidth", width}, {"exifImageHeight", height}, {"orientation", orientation}, {"city", "Karpacz"},
        {"country", "Polska"}}},
  };
}

} // namespace

int main() {
  // Landscape between 5:4 and 2.4:1 fits; portraits, squares and panoramas do not.
  CHECK(dash::fits_screen(asset(4000, 3000)));
  CHECK(dash::fits_screen(asset(1920, 1080)));
  CHECK(!dash::fits_screen(asset(3000, 4000)));
  CHECK(!dash::fits_screen(asset(3000, 3000)));
  CHECK(!dash::fits_screen(asset(9000, 3000)));
  // EXIF orientation 6 and 8: stored landscape, shown portrait, and the other way round.
  CHECK(!dash::fits_screen(asset(4000, 3000, "6")));
  CHECK(dash::fits_screen(asset(3000, 4000, "8")));
  CHECK(dash::fits_screen([] {
    json a = asset(4000, 3000);
    a["exifInfo"]["orientation"] = nullptr; // no orientation: as stored
    return a;
  }()));

  // Only plain timeline photos.
  for (const auto &[key, value] : std::initializer_list<std::pair<const char *, json>>{
           {"type", "VIDEO"}, {"visibility", "archive"}, {"visibility", "hidden"}, {"isTrashed", true},
           {"isOffline", true}, {"isArchived", true}}) {
    json a = asset(4000, 3000);
    a[key] = value;
    CHECK(!dash::fits_screen(a));
  }
  CHECK(!dash::fits_screen([] {
    json a = asset(4000, 3000);
    a.erase("exifInfo"); // size unknown
    return a;
  }()));

  // The Photo: served by our server, local time without Immich's false "Z", the city.
  const json photo = dash::photo_json(asset(4000, 3000), "0b5c2f1e-8d7a-4c3b-9e6f-1a2b3c4d5e6f.jpg");
  CHECK(photo["url"] == "/api/photos/0b5c2f1e-8d7a-4c3b-9e6f-1a2b3c4d5e6f.jpg");
  CHECK(photo["takenAt"] == "2025-08-14T18:42:10.000");
  CHECK(photo["place"] == "Karpacz");
  CHECK(photo.size() == 4); // id, url, takenAt, place: the Photo of types.ts

  json no_city = asset(4000, 3000);
  no_city["exifInfo"].erase("city");
  CHECK(dash::photo_json(no_city, "x.jpg")["place"] == "Polska");
  no_city["exifInfo"].erase("country");
  CHECK(dash::photo_json(no_city, "x.jpg")["place"].is_null());

  // Polish plurals.
  CHECK(dash::years_ago(1) == "Rok temu");
  CHECK(dash::years_ago(2) == "2 lata temu");
  CHECK(dash::years_ago(4) == "4 lata temu");
  CHECK(dash::years_ago(5) == "5 lat temu");
  CHECK(dash::years_ago(12) == "12 lat temu");
  CHECK(dash::years_ago(22) == "22 lata temu");
  CHECK(dash::years_ago(25) == "25 lat temu");

  // Preview files by content type; anything else is refused.
  CHECK(dash::preview_file("id", "image/jpeg") == "id.jpg");
  CHECK(dash::preview_file("id", "image/webp; charset=binary") == "id.webp");
  CHECK(!dash::preview_file("id", "application/json"));
  return 0;
}
