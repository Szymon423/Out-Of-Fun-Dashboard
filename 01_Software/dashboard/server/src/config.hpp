// Settings from <data dir>/config.json. The data dir survives updates (/data on the Pi).
// A missing file is created as a template to fill in:
//   {
//     "location": { "name": "Kraków" },
//     "immich": { "url": "http://192.168.1.20:2283", "apiKey": "..." },
//     "spotify": { "clientId": "..." },
//     "printer": { "host": "192.168.1.181", "serial": "...", "accessCode": "...", "model": "X2D" }
//   }
// With only a name the place is looked up (Open-Meteo geocoding); latitude and longitude
// may be given instead, then the name is only shown. The Immich API key needs the
// permissions asset.read, asset.view and memory.read. The Spotify client ID is that of the
// app on developer.spotify.com; the login itself is tools/spotify-login.py (spotify.hpp).
// The printer's access code and serial number are on its screen (settings, network).
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

struct ImmichSettings {
  std::string url; // without a trailing slash
  std::string api_key;
};

struct PrinterSettings {
  std::string host;
  std::string serial;
  std::string access_code;
  std::string name;  // shown on the page; "Drukarka" when not set
  std::string model; // e.g. "X2D"; the page shows "Bambu Lab <model>"
};

struct Config {
  Location location;
  std::optional<ImmichSettings> immich;
  std::optional<std::string> spotify_client_id;
  std::optional<PrinterSettings> printer;
};

/** Reads the config; throws std::runtime_error with the reason when it cannot be used. */
Config load_config(const std::filesystem::path &data_dir);

} // namespace dash
