// Weather from Open-Meteo (https://open-meteo.com: free, no key), refreshed every 15 min.
// The forecast is asked for in the place's time zone, which is also the Pi's, so its times
// are local wall-clock times like "2026-11-23T14:00", as web/src/lib/types.ts expects.
#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <thread>

#include "config.hpp"
#include "hub.hpp"

namespace dash {

/** The forecast request for a place. */
std::string open_meteo_url(double latitude, double longitude);

/**
 * Turns an Open-Meteo forecast into the Weather of web/src/lib/types.ts.
 * `now_local` is the local time as "YYYY-MM-DDTHH:MM": hours after it and days after its
 * date are the forecast. `updated` is an ISO 8601 time stamp of the reading.
 */
nlohmann::json parse_open_meteo(const nlohmann::json &forecast, const std::string &place, const std::string &now_local,
                                const std::string &updated);

/** Publishes "weather" in the background until destroyed. */
class WeatherProvider {
public:
  WeatherProvider(Hub &hub, Location location);

private:
  std::jthread thread_;
};

} // namespace dash
