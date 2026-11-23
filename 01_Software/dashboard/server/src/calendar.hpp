// Calendars (stage D5) from published ICS links: Outlook's "publish calendar", Google's
// secret iCal address, iCloud's public calendar. Read only, no login. Each link is fetched
// every 10 min; a link that fails keeps its last events.
//
// Events are expanded with libical from the start of the previous month to three months
// ahead: recurring series (RRULE), dropped dates (EXDATE), moved or cancelled single
// meetings (RECURRENCE-ID), and Outlook's Windows time zone names ("Central European
// Standard Time"), which the calendar itself defines in a VTIMEZONE.
//
// The links are secret (whoever has one sees the calendar), so they live only in config.json.
#pragma once

#include <ctime>
#include <nlohmann/json.hpp>
#include <string>
#include <thread>
#include <vector>

#include "config.hpp"
#include "hub.hpp"

namespace dash {

/**
 * The events of one ICS calendar that overlap [from, to), as CalendarEvent of
 * web/src/lib/types.ts: timed events in UTC ("…Z"), all-day ones as local midnight without
 * an offset. Throws std::runtime_error when the text is not a calendar.
 */
nlohmann::json ics_events(const std::string &ics, const CalendarSettings &source, std::time_t from, std::time_t to);

/**
 * An event description for the screen: Outlook's meeting footer (from the first line of
 * underscores on: "Microsoft Teams meeting", join links, dial-in numbers) cut off, runs of
 * empty lines squeezed, at most 2000 characters. Empty when nothing is left.
 */
std::string clean_description(std::string_view text);

/** Publishes "calendars" and "events" in the background until destroyed. */
class CalendarProvider {
public:
  CalendarProvider(Hub &hub, std::vector<CalendarSettings> calendars);

private:
  std::jthread thread_;
};

} // namespace dash
