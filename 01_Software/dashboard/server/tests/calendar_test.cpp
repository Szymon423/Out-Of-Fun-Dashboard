// Expanding an Outlook-style ICS calendar into CalendarEvents. The calendar is made up, in
// the shape Outlook publishes: a Windows time zone name defined by its own VTIMEZONE, a
// weekly series across the switch to summer time, a dropped date, a moved and a cancelled
// meeting of the series, an all-day event, and an event outside the window.
#include <string>

#include "calendar.hpp"
#include "check.hpp"

using nlohmann::json;

namespace {

const char *kIcs = "BEGIN:VCALENDAR\r\n"
                   "METHOD:PUBLISH\r\n"
                   "PRODID:Microsoft Exchange Server 2010\r\n"
                   "VERSION:2.0\r\n"
                   "BEGIN:VTIMEZONE\r\n"
                   "TZID:Central European Standard Time\r\n"
                   "BEGIN:STANDARD\r\n"
                   "DTSTART:16010101T030000\r\n"
                   "TZOFFSETFROM:+0200\r\n"
                   "TZOFFSETTO:+0100\r\n"
                   "RRULE:FREQ=YEARLY;INTERVAL=1;BYDAY=-1SU;BYMONTH=10\r\n"
                   "END:STANDARD\r\n"
                   "BEGIN:DAYLIGHT\r\n"
                   "DTSTART:16010101T020000\r\n"
                   "TZOFFSETFROM:+0100\r\n"
                   "TZOFFSETTO:+0200\r\n"
                   "RRULE:FREQ=YEARLY;INTERVAL=1;BYDAY=-1SU;BYMONTH=3\r\n"
                   "END:DAYLIGHT\r\n"
                   "END:VTIMEZONE\r\n"
                   // Tuesdays 09:00-09:30 local, 3 March to 14 April 2026.
                   "BEGIN:VEVENT\r\n"
                   "UID:series-1\r\n"
                   "SUMMARY:Daily zespołu\r\n"
                   "LOCATION:Microsoft Teams Meeting\r\n"
                   "DESCRIPTION:Plan na tydzień\\n\\n\\n\\nPytania? Pisz na czacie.\\n_______________\r\n"
                   " _____________\\nSpotkanie w aplikacji Microsoft Teams\\nDołącz: https://teams.example/join\r\n"
                   "DTSTART;TZID=Central European Standard Time:20260303T090000\r\n"
                   "DTEND;TZID=Central European Standard Time:20260303T093000\r\n"
                   "RRULE:FREQ=WEEKLY;UNTIL=20260414T070000Z;INTERVAL=1;BYDAY=TU;WKST=MO\r\n"
                   "EXDATE;TZID=Central European Standard Time:20260317T090000\r\n"
                   "END:VEVENT\r\n"
                   // 24 March moved to 13:00.
                   "BEGIN:VEVENT\r\n"
                   "UID:series-1\r\n"
                   "RECURRENCE-ID;TZID=Central European Standard Time:20260324T090000\r\n"
                   "SUMMARY:Daily zespołu (później)\r\n"
                   "DTSTART;TZID=Central European Standard Time:20260324T130000\r\n"
                   "DTEND;TZID=Central European Standard Time:20260324T133000\r\n"
                   "END:VEVENT\r\n"
                   // 31 March cancelled.
                   "BEGIN:VEVENT\r\n"
                   "UID:series-1\r\n"
                   "RECURRENCE-ID;TZID=Central European Standard Time:20260331T090000\r\n"
                   "SUMMARY:Daily zespołu\r\n"
                   "STATUS:CANCELLED\r\n"
                   "DTSTART;TZID=Central European Standard Time:20260331T090000\r\n"
                   "DTEND;TZID=Central European Standard Time:20260331T093000\r\n"
                   "END:VEVENT\r\n"
                   "BEGIN:VEVENT\r\n"
                   "UID:all-day-1\r\n"
                   "SUMMARY:Urlop\r\n"
                   "DTSTART;VALUE=DATE:20260320\r\n"
                   "DTEND;VALUE=DATE:20260321\r\n"
                   "X-MICROSOFT-CDO-ALLDAYEVENT:TRUE\r\n"
                   "END:VEVENT\r\n"
                   "BEGIN:VEVENT\r\n"
                   "UID:outside-1\r\n"
                   "SUMMARY:Za oknem\r\n"
                   "DTSTART;TZID=Central European Standard Time:20260901T100000\r\n"
                   "DTEND;TZID=Central European Standard Time:20260901T110000\r\n"
                   "END:VEVENT\r\n"
                   "END:VCALENDAR\r\n";

std::time_t utc(int y, int m, int d) {
  std::tm tm{};
  tm.tm_year = y - 1900;
  tm.tm_mon = m - 1;
  tm.tm_mday = d;
  return timegm(&tm);
}

const json *find(const json &events, const std::string &start) {
  for (const json &e : events) {
    if (e["start"] == start) return &e;
  }
  return nullptr;
}

} // namespace

int main() {
  const dash::CalendarSettings m4t{"m4t", "M4T", "#0a84ff", "https://example.invalid/calendar.ics"};
  const json events = dash::ics_events(kIcs, m4t, utc(2026, 3, 1), utc(2026, 5, 1));

  // The series: 09:00 is 08:00 UTC in winter and 07:00 UTC from 29 March.
  CHECK(find(events, "2026-03-03T08:00:00Z"));
  CHECK(find(events, "2026-03-10T08:00:00Z"));
  CHECK(!find(events, "2026-03-17T08:00:00Z")); // EXDATE
  CHECK(!find(events, "2026-03-24T08:00:00Z")); // moved ...
  CHECK(find(events, "2026-03-24T12:00:00Z"));  // ... to 13:00
  CHECK(!find(events, "2026-03-31T07:00:00Z")); // cancelled
  CHECK(find(events, "2026-04-07T07:00:00Z"));
  CHECK(find(events, "2026-04-14T07:00:00Z")); // UNTIL is inclusive
  CHECK(!find(events, "2026-04-21T07:00:00Z"));

  const json &first = *find(events, "2026-03-03T08:00:00Z");
  CHECK(first["end"] == "2026-03-03T08:30:00Z");
  CHECK(first["title"] == "Daily zespołu");
  CHECK(first["location"] == "Microsoft Teams Meeting");
  CHECK(first["calendarId"] == "m4t" && first["calendar"] == "M4T" && first["color"] == "#0a84ff");
  CHECK(first["allDay"] == false);
  CHECK(first["description"] == "Plan na tydzień\n\nPytania? Pisz na czacie."); // without the Teams footer
  CHECK((*find(events, "2026-03-24T12:00:00Z"))["title"] == "Daily zespołu (później)");

  // All-day: local midnight without an offset, the end exclusive.
  const json *leave = find(events, "2026-03-20T00:00:00");
  CHECK(leave && (*leave)["allDay"] == true && (*leave)["end"] == "2026-03-21T00:00:00");
  CHECK(!leave->contains("location"));
  CHECK(!leave->contains("description"));

  // Descriptions: the footer goes, empty lines squeeze, long text ends whole.
  CHECK(dash::clean_description("  \n\n") == "");
  CHECK(dash::clean_description("Linia 1\r\n\r\n\r\nLinia 2\r\n") == "Linia 1\n\nLinia 2");
  CHECK(dash::clean_description("________________\nTeams") == "");
  const std::string long_text = dash::clean_description(std::string(3000, 'x') + "ż");
  CHECK(long_text.size() <= 2000 + 3 && long_text.ends_with("…"));
  const std::string polish = dash::clean_description(std::string(1999, 'x') + "żółw");
  CHECK(polish == std::string(1999, 'x') + "…"); // "ż" does not fit whole: dropped, not cut in half

  // Five of the series, the moved one, the all-day one; nothing from September.
  CHECK(events.size() == 7 - 1); // 3 Mar, 10 Mar, 24 Mar moved, 7 Apr, 14 Apr, leave
  bool ids_unique = true;
  for (std::size_t i = 0; i < events.size(); ++i) {
    for (std::size_t j = i + 1; j < events.size(); ++j) ids_unique &= events[i]["id"] != events[j]["id"];
  }
  CHECK(ids_unique);

  // Not a calendar at all.
  bool threw = false;
  try {
    (void)dash::ics_events("<html>Sign in</html>", m4t, utc(2026, 3, 1), utc(2026, 5, 1));
  } catch (const std::runtime_error &) {
    threw = true;
  }
  CHECK(threw);
  return 0;
}
