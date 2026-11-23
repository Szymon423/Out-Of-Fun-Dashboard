#include "lights.hpp"

#include <fstream>
#include <regex>

#include "log.hpp"

namespace dash {

using nlohmann::json;

namespace {

json defaults() {
  return json::array({
      {{"id", "desk-left"}, {"name", "Lampka lewa"}, {"kind", "switch"}, {"on", false}},
      {{"id", "desk-right"}, {"name", "Lampka prawa"}, {"kind", "switch"}, {"on", false}},
      {{"id", "strip"},
       {"name", "Listwa LED"},
       {"kind", "rgbw"},
       {"on", false},
       {"brightness", 60},
       {"color", "#ff7a3d"},
       {"white", 20}},
  });
}

bool percent(const json &value) {
  return value.is_number() && value.get<double>() >= 0 && value.get<double>() <= 100;
}

} // namespace

Lights::Lights(Hub &hub, std::filesystem::path file) : hub_(hub), file_(std::move(file)), lights_(defaults()) {
  // Keep the saved on/off and colours of lights that still exist; names come from the code.
  try {
    if (std::filesystem::exists(file_)) {
      const json saved = json::parse(std::ifstream(file_));
      for (json &light : lights_) {
        for (const json &old : saved) {
          if (old.value("id", "") != light["id"].get<std::string>()) continue;
          for (const char *key : {"on", "brightness", "color", "white"}) {
            if (light.contains(key) && old.contains(key)) light[key] = old[key];
          }
        }
      }
    }
  } catch (const std::exception &e) {
    log_warning("lights: ignoring {}: {}", file_.string(), e.what());
  }
  hub_.publish("lights", lights_);
}

Lights::Result Lights::apply(const std::string &id, const json &change, std::string &error) {
  static const std::regex kColor("^#[0-9a-fA-F]{6}$");
  const std::lock_guard lock(mutex_);

  const auto light = std::find_if(lights_.begin(), lights_.end(), [&](const json &l) { return l["id"] == id; });
  if (light == lights_.end()) return Result::unknown_light;
  if (!change.is_object() || change.empty()) {
    error = "expected a JSON object with on, brightness, color or white";
    return Result::bad_request;
  }

  const bool rgbw = (*light)["kind"] == "rgbw";
  json updated = *light;
  for (const auto &[key, value] : change.items()) {
    if (key == "on" && value.is_boolean()) {
      updated[key] = value;
    } else if ((key == "brightness" || key == "white") && rgbw && percent(value)) {
      updated[key] = static_cast<int>(std::lround(value.get<double>()));
    } else if (key == "color" && rgbw && value.is_string() && std::regex_match(value.get<std::string>(), kColor)) {
      updated[key] = value;
    } else {
      error = std::format("bad or unsupported field \"{}\" for light {}", key, id);
      return Result::bad_request;
    }
  }

  *light = std::move(updated);
  save();
  hub_.publish("lights", lights_);
  return Result::ok;
}

void Lights::save() const {
  const auto tmp = std::filesystem::path(file_).concat(".tmp");
  {
    std::ofstream out(tmp);
    out << lights_.dump(2) << '\n';
    if (!out) {
      log_warning("lights: cannot write {}", tmp.string());
      return;
    }
  }
  std::error_code ec;
  std::filesystem::rename(tmp, file_, ec);
  if (ec) log_warning("lights: cannot save {}: {}", file_.string(), ec.message());
}

} // namespace dash
