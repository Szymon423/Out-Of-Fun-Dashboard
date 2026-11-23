#include "weather.hpp"

#include <algorithm>
#include <condition_variable>
#include <ctime>
#include <format>
#include <mutex>

#include "fetch.hpp"
#include "log.hpp"

namespace dash {

using nlohmann::json;
using namespace std::chrono_literals;

namespace {

constexpr std::size_t kHours = 24;
constexpr std::size_t kDays = 7;
constexpr auto kRefresh = 15min;
constexpr auto kFirstRetry = 5s; // then twice as long each time; at boot the network is a second behind

double num(const json &j) { return j.is_number() ? j.get<double>() : 0.0; }
int whole(const json &j) { return j.is_number() ? static_cast<int>(std::lround(j.get<double>())) : 0; }

std::string local_now() {
  const std::time_t t = std::time(nullptr);
  std::tm tm{};
  localtime_r(&t, &tm);
  char buf[32];
  std::strftime(buf, sizeof buf, "%Y-%m-%dT%H:%M", &tm);
  return buf;
}

std::string utc_now_iso() {
  const std::time_t t = std::time(nullptr);
  std::tm tm{};
  gmtime_r(&t, &tm);
  char buf[32];
  std::strftime(buf, sizeof buf, "%Y-%m-%dT%H:%M:%SZ", &tm);
  return buf;
}

struct NotFound : std::runtime_error {
  using std::runtime_error::runtime_error;
};

/** Latitude and longitude of a place name, from Open-Meteo's geocoding. */
Location geocode(const Location &location) {
  const auto body = http_get(std::format("https://geocoding-api.open-meteo.com/v1/search?name={}&count=1&language=pl"
                                         "&format=json",
                                         url_encode(location.name)));
  const auto doc = json::parse(body);
  const auto results = doc.find("results");
  if (results == doc.end() || results->empty()) {
    throw NotFound(std::format("no place called \"{}\"", location.name));
  }
  const auto &first = results->at(0);
  Location found = location;
  found.latitude = first.at("latitude").get<double>();
  found.longitude = first.at("longitude").get<double>();
  log_info("weather: \"{}\" is {}, {} at {:.4f}, {:.4f}", location.name, first.value("name", "?"),
           first.value("admin1", first.value("country", "?")), *found.latitude, *found.longitude);
  return found;
}

/** Sleeps, but wakes at once when the provider is being destroyed; false then. */
bool sleep_for(std::stop_token stop, std::chrono::seconds duration) {
  std::mutex mutex;
  std::condition_variable_any cv;
  std::unique_lock lock(mutex);
  cv.wait_for(lock, stop, duration, [] { return false; });
  return !stop.stop_requested();
}

void run(std::stop_token stop, Hub &hub, Location location) {
  std::chrono::seconds retry = kFirstRetry;
  auto backoff = [&] {
    const auto wait = retry;
    retry = std::min<std::chrono::seconds>(retry * 2, kRefresh);
    return wait;
  };

  while (!location.latitude) {
    try {
      location = geocode(location);
    } catch (const NotFound &e) {
      log_error("weather: {}; fix location.name in config.json", e.what());
      return;
    } catch (const std::exception &e) {
      log_warning("weather: looking up the place failed, retrying: {}", e.what());
      if (!sleep_for(stop, backoff())) return;
    }
  }
  retry = kFirstRetry;

  const std::string url = open_meteo_url(*location.latitude, *location.longitude);
  bool failing = true; // log the first success and every recovery, not each refresh
  while (!stop.stop_requested()) {
    std::chrono::seconds next = kRefresh;
    try {
      const auto forecast = json::parse(http_get(url));
      hub.publish("weather", parse_open_meteo(forecast, location.name, local_now(), utc_now_iso()));
      if (failing) log_info("weather: updated, next every {} min", kRefresh.count());
      failing = false;
      retry = kFirstRetry;
    } catch (const std::exception &e) {
      next = backoff();
      log_warning("weather: update failed, retrying in {} s: {}", next.count(), e.what());
      failing = true;
    }
    if (!sleep_for(stop, next)) return;
  }
}

} // namespace

std::string open_meteo_url(double latitude, double longitude) {
  return std::format("https://api.open-meteo.com/v1/forecast?latitude={:.4f}&longitude={:.4f}"
                     "&current=temperature_2m,apparent_temperature,relative_humidity_2m,is_day,weather_code,"
                     "wind_speed_10m"
                     "&hourly=temperature_2m,weather_code,precipitation_probability,is_day"
                     "&daily=weather_code,temperature_2m_max,temperature_2m_min,sunrise,sunset,"
                     "precipitation_probability_max"
                     "&timezone=auto&forecast_days={}",
                     latitude, longitude, kDays + 1);
}

json parse_open_meteo(const json &forecast, const std::string &place, const std::string &now_local,
                      const std::string &updated) {
  const json &current = forecast.at("current");
  const json now = {
      {"temp", num(current.at("temperature_2m"))},
      {"feelsLike", num(current.at("apparent_temperature"))},
      {"code", whole(current.at("weather_code"))},
      {"isDay", whole(current.at("is_day")) == 1},
      {"windKmh", num(current.at("wind_speed_10m"))},
      {"humidity", whole(current.at("relative_humidity_2m"))},
  };

  const json &daily = forecast.at("daily");
  const json &dates = daily.at("time");
  const std::string today_date = now_local.substr(0, 10);
  std::size_t today = 0;
  while (today < dates.size() && dates[today].get<std::string>() < today_date) ++today;
  if (today >= dates.size()) throw std::runtime_error("the forecast does not reach today");

  const json today_json = {
      {"min", num(daily.at("temperature_2m_min")[today])},
      {"max", num(daily.at("temperature_2m_max")[today])},
      {"sunrise", daily.at("sunrise")[today]},
      {"sunset", daily.at("sunset")[today]},
      {"precipProb", whole(daily.at("precipitation_probability_max")[today])},
  };

  json days = json::array();
  for (std::size_t i = today + 1; i < dates.size() && days.size() < kDays; ++i) {
    days.push_back({
        {"date", dates[i]},
        {"code", whole(daily.at("weather_code")[i])},
        {"min", num(daily.at("temperature_2m_min")[i])},
        {"max", num(daily.at("temperature_2m_max")[i])},
        {"precipProb", whole(daily.at("precipitation_probability_max")[i])},
    });
  }

  const json &hourly = forecast.at("hourly");
  const json &times = hourly.at("time");
  json hours = json::array();
  for (std::size_t i = 0; i < times.size() && hours.size() < kHours; ++i) {
    if (times[i].get<std::string>() <= now_local) continue; // from the coming full hour
    hours.push_back({
        {"time", times[i]},
        {"temp", num(hourly.at("temperature_2m")[i])},
        {"code", whole(hourly.at("weather_code")[i])},
        {"isDay", whole(hourly.at("is_day")[i]) == 1},
        {"precipProb", whole(hourly.at("precipitation_probability")[i])},
    });
  }

  return {
      {"updated", updated}, {"place", place}, {"now", now}, {"today", today_json}, {"hours", hours}, {"days", days},
  };
}

WeatherProvider::WeatherProvider(Hub &hub, Location location)
    : thread_([&hub, location](std::stop_token stop) { run(stop, hub, location); }) {}

} // namespace dash
