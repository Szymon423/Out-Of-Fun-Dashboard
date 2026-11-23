#include "config.hpp"

#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>

#include "log.hpp"

namespace dash {

using nlohmann::json;

namespace {

constexpr const char *kTemplate = R"({
  "location": { "name": "" }
}
)";

} // namespace

Config load_config(const std::filesystem::path &data_dir) {
  const auto path = data_dir / "config.json";
  if (!std::filesystem::exists(path)) {
    std::ofstream(path) << kTemplate;
    log_warning("{} did not exist, wrote a template to fill in", path.string());
  }

  json doc;
  try {
    doc = json::parse(std::ifstream(path), nullptr, true, true); // comments allowed
  } catch (const json::exception &e) {
    throw std::runtime_error(std::format("{}: {}", path.string(), e.what()));
  }

  Config config;
  if (const auto loc = doc.find("location"); loc != doc.end() && loc->is_object()) {
    config.location.name = loc->value("name", "");
    const bool has_lat = loc->contains("latitude");
    const bool has_lon = loc->contains("longitude");
    if (has_lat != has_lon) {
      throw std::runtime_error(std::format("{}: give both latitude and longitude, or neither", path.string()));
    }
    if (has_lat) {
      config.location.latitude = loc->at("latitude").get<double>();
      config.location.longitude = loc->at("longitude").get<double>();
    }
  }
  if (const auto immich = doc.find("immich"); immich != doc.end() && immich->is_object()) {
    ImmichSettings settings{immich->value("url", ""), immich->value("apiKey", "")};
    while (settings.url.ends_with('/')) settings.url.pop_back();
    if (settings.url.empty() || settings.api_key.empty()) {
      log_error("{}: immich needs both url and apiKey; no photos", path.string()); // weather still works
    } else {
      config.immich = std::move(settings);
    }
  }
  if (const auto printer = doc.find("printer"); printer != doc.end() && printer->is_object()) {
    PrinterSettings settings{printer->value("host", ""), printer->value("serial", ""),
                             printer->value("accessCode", ""), printer->value("name", "Drukarka"),
                             printer->value("model", "")};
    if (settings.host.empty() || settings.serial.empty() || settings.access_code.empty()) {
      log_error("{}: printer needs host, serial and accessCode; no printer", path.string());
    } else {
      config.printer = std::move(settings);
    }
  }
  if (const auto spotify = doc.find("spotify"); spotify != doc.end() && spotify->is_object()) {
    if (std::string id = spotify->value("clientId", ""); !id.empty()) config.spotify_client_id = std::move(id);
  }
  return config;
}

} // namespace dash
