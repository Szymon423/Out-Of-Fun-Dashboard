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
  return config;
}

} // namespace dash
