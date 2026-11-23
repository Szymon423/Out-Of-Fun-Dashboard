// Settings from <data dir>/config.json. The data dir survives updates (/data on the Pi).
// A missing file is created as a template to fill in:
//   { "location": { "name": "Kraków" } }
// With only a name the place is looked up (Open-Meteo geocoding); latitude and longitude
// may be given instead, then the name is only shown.
#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace dash {

struct Location {
  std::string name;
  std::optional<double> latitude;
  std::optional<double> longitude;

  bool empty() const { return name.empty() && !latitude; }
};

struct Config {
  Location location;
};

/** Reads the config; throws std::runtime_error with the reason when it cannot be used. */
Config load_config(const std::filesystem::path &data_dir);

} // namespace dash
