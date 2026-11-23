#include "calendar.hpp"

#include <libical/ical.h>

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <stdexcept>

#include "fetch.hpp"
#include "log.hpp"

namespace dash {

using nlohmann::json;
using namespace std::chrono_literals;

namespace {

constexpr auto kRefresh = 10min;
constexpr auto kFirstRetry = 5s; // then twice as long each time; at boot the network is a second behind

struct ComponentDeleter {
  void operator()(icalcomponent *c) const { icalcomponent_free(c); }
};

std::string iso_utc(std::time_t t) {
  std::tm tm{};
  gmtime_r(&t, &tm);
  char buf[32];
  std::strftime(buf, sizeof buf, "%Y-%m-%dT%H:%M:%SZ", &tm);
  return buf;
}

/** An all-day date (libical gives it as UTC midnight) as local midnight without an offset. */
std::string iso_day(std::time_t t) {
  std::tm tm{};
  gmtime_r(&t, &tm);
  char buf[32];
  std::strftime(buf, sizeof buf, "%Y-%m-%dT00:00:00", &tm);
  return buf;
}

/** The time zone a property's TZID names: the calendar's own VTIMEZONE first, then the system's. */
const icaltimezone *zone_of(icalcomponent *calendar, icalproperty *prop) {
  icalparameter *param = prop ? icalproperty_get_first_parameter(prop, ICAL_TZID_PARAMETER) : nullptr;
  if (!param) return nullptr;
  const char *tzid = icalparameter_get_tzid(param);
  if (const icaltimezone *own = icalcomponent_get_timezone(calendar, tzid)) return own;
  return icaltimezone_get_builtin_timezone(tzid);
}

/** A DTSTART-like time as time_t. Floating times (no zone) count as the Pi's local time. */
std::time_t as_time_t(icaltimetype t, const icaltimezone *zone) {
  if (t.is_date || icaltime_is_utc(t)) return icaltime_as_timet_with_zone(t, icaltimezone_get_utc_timezone());
  if (zone) return icaltime_as_timet_with_zone(t, zone);
  if (t.zone) return icaltime_as_timet_with_zone(t, t.zone);
  std::tm tm{};
  tm.tm_year = t.year - 1900;
  tm.tm_mon = t.month - 1;
  tm.tm_mday = t.day;
  tm.tm_hour = t.hour;
  tm.tm_min = t.minute;
  tm.tm_sec = t.second;
  tm.tm_isdst = -1;
  return std::mktime(&tm);
}

/** The original start of a moved or cancelled meeting of a series. */
std::time_t recurrence_id(icalcomponent *calendar, icalcomponent *event) {
  icalproperty *prop = icalcomponent_get_first_property(event, ICAL_RECURRENCEID_PROPERTY);
  return as_time_t(icalproperty_get_recurrenceid(prop), zone_of(calendar, prop));
}

std::string text_or(const char *value, const char *fallback) { return value && *value ? value : fallback; }

struct Expansion {
  const CalendarSettings *source;
  std::time_t from;
  std::time_t to;
  std::set<std::time_t> moved; // instances of this series replaced by their own VEVENT
  json *out;
};

void add_instance(icalcomponent *event, icaltime_span *span, void *data) {
  auto *e = static_cast<Expansion *>(data);
  if (e->moved.contains(span->start)) return;
  if (span->end <= e->from || span->start >= e->to) return; // only touches the window's edge
  const bool all_day = icalcomponent_get_dtstart(event).is_date;
  json item = {
      {"id", text_or(icalcomponent_get_uid(event), "?") + "/" + std::to_string(span->start)},
      {"calendarId", e->source->id},
      {"title", text_or(icalcomponent_get_summary(event), "(bez tytułu)")},
      {"start", all_day ? iso_day(span->start) : iso_utc(span->start)},
      {"end", all_day ? iso_day(span->end) : iso_utc(span->end)},
      {"allDay", all_day},
      {"calendar", e->source->name},
      {"color", e->source->color},
  };
  if (const char *location = icalcomponent_get_location(event); location && *location) item["location"] = location;
  if (const char *description = icalcomponent_get_description(event); description) {
    if (std::string text = clean_description(description); !text.empty()) item["description"] = std::move(text);
  }
  e->out->push_back(std::move(item));
}

/** Start of the previous month and of the month three months on, local time. */
std::pair<std::time_t, std::time_t> window() {
  const std::time_t now = std::time(nullptr);
  std::tm tm{};
  localtime_r(&now, &tm);
  tm.tm_mday = 1;
  tm.tm_hour = tm.tm_min = tm.tm_sec = 0;
  tm.tm_isdst = -1;
  std::tm from = tm;
  from.tm_mon -= 1;
  std::tm to = tm;
  to.tm_mon += 3;
  return {std::mktime(&from), std::mktime(&to)};
}

bool sleep_for(std::stop_token stop, std::chrono::seconds duration) {
  std::mutex mutex;
  std::condition_variable_any cv;
  std::unique_lock lock(mutex);
  cv.wait_for(lock, stop, duration, [] { return false; });
  return !stop.stop_requested();
}

void run(std::stop_token stop, Hub &hub, const std::vector<CalendarSettings> &calendars) {
  json sources = json::array();
  for (const auto &c : calendars) sources.push_back({{"id", c.id}, {"name", c.name}, {"color", c.color}});
  hub.publish("calendars", sources);

  std::map<std::string, json> events_of; // by calendar id; the last good read of each
  enum class Health { unknown, ok, failing };
  std::map<std::string, Health> health; // log changes only, not every refresh
  std::chrono::seconds retry = kFirstRetry;

  while (!stop.stop_requested()) {
    const auto [from, to] = window();
    bool all_ok = true;
    for (const auto &calendar : calendars) {
      try {
        events_of[calendar.id] = ics_events(http_get(calendar.url, 30s), calendar, from, to);
        if (health[calendar.id] != Health::ok) {
          log_info("calendar: {}: {} events, refreshed every {} min", calendar.name, events_of[calendar.id].size(),
                   kRefresh.count());
        }
        health[calendar.id] = Health::ok;
      } catch (const std::exception &e) {
        all_ok = false;
        if (health[calendar.id] != Health::failing) {
          // The link is secret: the error text names it, the journal must not.
          std::string reason = e.what();
          for (auto at = reason.find(calendar.url); at != std::string::npos; at = reason.find(calendar.url)) {
            reason.replace(at, calendar.url.size(), "<link>");
          }
          log_warning("calendar: {} failed, keeping its last events: {}", calendar.name, reason);
        }
        health[calendar.id] = Health::failing;
      }
    }

    json events = json::array();
    for (const auto &calendar : calendars) {
      if (const auto it = events_of.find(calendar.id); it != events_of.end()) {
        events.insert(events.end(), it->second.begin(), it->second.end());
      }
    }
    std::sort(events.begin(), events.end(), [](const json &a, const json &b) { return a["start"] < b["start"]; });
    if (!events.empty() || all_ok) hub.publish("events", events);

    std::chrono::seconds next = kRefresh;
    if (!all_ok) {
      next = retry;
      retry = std::min<std::chrono::seconds>(retry * 2, kRefresh);
    } else {
      retry = kFirstRetry;
    }
    if (!sleep_for(stop, next)) return;
  }
}

} // namespace

std::string clean_description(std::string_view text) {
  constexpr std::size_t kMax = 2000;
  std::string out;
  int empty_run = 0;
  std::size_t pos = 0;
  while (pos <= text.size()) {
    const std::size_t eol = std::min(text.find('\n', pos), text.size());
    std::string_view line = text.substr(pos, eol - pos);
    pos = eol + 1;
    while (!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t')) line.remove_suffix(1);
    if (line.find("__________") != std::string_view::npos) break; // Outlook's meeting footer starts here
    if (line.empty()) {
      ++empty_run;
      continue;
    }
    if (!out.empty()) out += empty_run > 0 ? "\n\n" : "\n";
    empty_run = 0;
    out += line;
    if (out.size() > kMax) {
      out.resize(kMax);
      while (!out.empty() && (static_cast<unsigned char>(out.back()) & 0xC0) == 0x80) out.pop_back(); // whole UTF-8 only
      if (!out.empty() && static_cast<unsigned char>(out.back()) >= 0xC0) out.pop_back();
      out += "…";
      break;
    }
  }
  return out;
}

json ics_events(const std::string &ics, const CalendarSettings &source, std::time_t from, std::time_t to) {
  const std::unique_ptr<icalcomponent, ComponentDeleter> calendar(icalparser_parse_string(ics.c_str()));
  if (!calendar || icalcomponent_isa(calendar.get()) != ICAL_VCALENDAR_COMPONENT) {
    throw std::runtime_error(ics.empty() ? "the link gave an empty answer" : "not an iCalendar file");
  }
  icalcomponent *cal = calendar.get();

  // Meetings moved or cancelled within a series come as their own VEVENT with the
  // RECURRENCE-ID of the date they replace; that date must not come from the series.
  std::map<std::string, std::set<std::time_t>> moved;
  for (icalcomponent *e = icalcomponent_get_first_component(cal, ICAL_VEVENT_COMPONENT); e;
       e = icalcomponent_get_next_component(cal, ICAL_VEVENT_COMPONENT)) {
    if (icalcomponent_get_first_property(e, ICAL_RECURRENCEID_PROPERTY)) {
      moved[text_or(icalcomponent_get_uid(e), "")].insert(recurrence_id(cal, e));
    }
  }

  json out = json::array();
  const icaltimezone *utc = icaltimezone_get_utc_timezone();
  const icaltimetype start = icaltime_from_timet_with_zone(from, 0, utc);
  const icaltimetype end = icaltime_from_timet_with_zone(to, 0, utc);
  for (icalcomponent *e = icalcomponent_get_first_component(cal, ICAL_VEVENT_COMPONENT); e;
       e = icalcomponent_get_next_component(cal, ICAL_VEVENT_COMPONENT)) {
    if (icalcomponent_get_status(e) == ICAL_STATUS_CANCELLED) continue;
    const bool is_override = icalcomponent_get_first_property(e, ICAL_RECURRENCEID_PROPERTY) != nullptr;
    Expansion expansion{&source, from, to, {}, &out};
    if (!is_override) {
      if (const auto it = moved.find(text_or(icalcomponent_get_uid(e), "")); it != moved.end()) {
        expansion.moved = it->second;
      }
    }
    icalcomponent_foreach_recurrence(e, start, end, add_instance, &expansion);
  }
  return out;
}

CalendarProvider::CalendarProvider(Hub &hub, std::vector<CalendarSettings> calendars)
    : thread_([&hub, calendars = std::move(calendars)](std::stop_token stop) { run(stop, hub, calendars); }) {}

} // namespace dash
