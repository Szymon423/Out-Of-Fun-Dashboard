// The three desk lights. There is no lighting hardware yet, so this is a mock: the state
// lives in memory and in <data dir>/lights.json, so it survives a restart. A real backend
// (Home Assistant, MQTT, ...) will replace it behind the same POST /api/lights/{id}.
#pragma once

#include <filesystem>
#include <mutex>
#include <nlohmann/json.hpp>
#include <string>

#include "hub.hpp"

namespace dash {

class Lights {
public:
  Lights(Hub &hub, std::filesystem::path file);

  enum class Result { ok, unknown_light, bad_request };

  /**
   * Applies a change from the frontend: {"on": bool} and, for the RGBW strip, also
   * "brightness" 0-100, "color" "#rrggbb", "white" 0-100. On bad_request, `error` says why.
   */
  Result apply(const std::string &id, const nlohmann::json &change, std::string &error);

private:
  void save() const;

  Hub &hub_;
  std::filesystem::path file_;
  std::mutex mutex_;
  nlohmann::json lights_; // array of Light, as in web/src/lib/types.ts
};

} // namespace dash
