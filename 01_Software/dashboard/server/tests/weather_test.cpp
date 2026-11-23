// Parsing a real Open-Meteo answer (tests/open-meteo.json, Warsaw, read on 2026-11-23 at
// 15:00) into the Weather of web/src/lib/types.ts.
#include <fstream>

#include "check.hpp"
#include "weather.hpp"

using nlohmann::json;

int main(int argc, char **argv) {
  CHECK(argc == 2);
  const json forecast = json::parse(std::ifstream(argv[1]));

  const json w = dash::parse_open_meteo(forecast, "Warszawa", "2026-11-23T15:20", "2026-11-23T14:20:00Z");
  CHECK(w["place"] == "Warszawa");
  CHECK(w["updated"] == "2026-11-23T14:20:00Z");

  const json &now = w["now"];
  CHECK(now["temp"] == 2.0);
  CHECK(now["feelsLike"] == -2.6);
  CHECK(now["code"] == 51);
  CHECK(now["isDay"] == true);
  CHECK(now["windKmh"] == 17.3);
  CHECK(now["humidity"] == 90);

  // Today is the first day of the forecast.
  const json &today = w["today"];
  CHECK(today["min"].get<double>() <= today["max"].get<double>());
  CHECK(today["sunrise"].get<std::string>().starts_with("2026-11-23T07:"));
  CHECK(today["sunset"].get<std::string>().starts_with("2026-11-23T15:"));
  CHECK(today["precipProb"].is_number_integer());

  // Hours start with the coming full hour, 24 of them.
  const json &hours = w["hours"];
  CHECK(hours.size() == 24);
  CHECK(hours[0]["time"] == "2026-11-23T16:00");
  CHECK(hours[23]["time"] == "2026-11-24T15:00");
  CHECK(hours[0]["isDay"] == false); // after sunset
  CHECK(hours[0].contains("temp") && hours[0].contains("code") && hours[0].contains("precipProb"));

  // Days start tomorrow, 7 of them.
  const json &days = w["days"];
  CHECK(days.size() == 7);
  CHECK(days[0]["date"] == "2026-11-24");
  CHECK(days[6]["date"] == "2026-11-30");
  for (const json &d : days) CHECK(d["min"].get<double>() <= d["max"].get<double>());

  // Later the same day the past hours drop out; after midnight "today" moves on.
  const json late = dash::parse_open_meteo(forecast, "Warszawa", "2026-11-23T23:59", "x");
  CHECK(late["hours"][0]["time"] == "2026-11-24T00:00");
  const json tomorrow = dash::parse_open_meteo(forecast, "Warszawa", "2026-11-24T00:05", "x");
  CHECK(tomorrow["days"][0]["date"] == "2026-11-25");
  CHECK(tomorrow["days"].size() == 6);

  // Fields asked for later than the saved answer: 0 or "steady" when missing, not an error.
  CHECK(w["now"]["gustKmh"] == 0.0 && w["today"]["uvPeak"].is_null());
  CHECK(w["today"]["pressureTrend"] == "steady");

  // ... and read when present: UV peak of today, the pressure in three hours, rain amounts.
  json more = forecast;
  more["current"]["wind_gusts_10m"] = 34.6;
  more["current"]["wind_direction_10m"] = 247.4;
  more["current"]["pressure_msl"] = 1012.4;
  const std::size_t n = more["hourly"]["time"].size();
  json uv = json::array(), pressure = json::array(), rain = json::array();
  for (std::size_t i = 0; i < n; ++i) {
    const std::string t = more["hourly"]["time"][i];
    uv.push_back(t == "2026-11-23T12:00" ? 1.35 : t.starts_with("2026-11-23T1") ? 0.5 : 0.0);
    pressure.push_back(t == "2026-11-23T18:00" ? 1009.0 : 1012.0);
    rain.push_back(t == "2026-11-23T16:00" ? 0.44 : 0.0);
  }
  more["hourly"]["uv_index"] = uv;
  more["hourly"]["pressure_msl"] = pressure;
  more["hourly"]["precipitation"] = rain;
  more["daily"]["uv_index_max"] = json::array({1.35, 0.9, 0.9, 0.9, 0.9, 0.9, 0.9, 0.9});
  more["daily"]["precipitation_sum"] = json::array({2.24, 0, 0, 0, 0, 0, 0, 0});
  more["daily"]["daylight_duration"] = json::array({30300.0, 0, 0, 0, 0, 0, 0, 0});
  const json m = dash::parse_open_meteo(more, "Warszawa", "2026-11-23T15:20", "x");
  CHECK(m["now"]["gustKmh"] == 34.6);
  CHECK(m["now"]["windDir"] == 247);
  CHECK(m["now"]["pressure"] == 1012);
  CHECK(m["today"]["uvMax"] == 1.4);
  CHECK(m["today"]["uvPeak"] == "12:00");
  CHECK(m["today"]["precipMm"] == 2.2);
  CHECK(m["today"]["daylightMin"] == 505);
  CHECK(m["today"]["pressureTrend"] == "down"); // 1012.4 now, 1009 at 18:00 (16:00 + 2 h)
  CHECK(m["hours"][0]["precipMm"] == 0.4);
  return 0;
}
