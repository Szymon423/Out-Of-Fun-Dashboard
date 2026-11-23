// Bambu Lab printer (stage D4b), read over its local MQTT broker: ssl://<host>:8883, user
// "bblp", the access code from the printer's screen as the password. This works in the
// printer's normal cloud mode; LAN-only and developer mode are not needed for reading.
//
// The printer publishes its state on device/<serial>/report. The first answer to a "pushall"
// request is the whole state; after that come partial updates, about one a second while it
// prints, which are merged into the whole (merge_report). Read only: nothing is ever sent to
// the printer except pushall, which is the same status request Bambu Studio makes.
//
// "printer" is published when the state, the job, the AMS or a message changes, and for
// temperatures (whole degrees) at most every 3 s, so the page is not redrawn every second.
#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <thread>

#include "config.hpp"
#include "hub.hpp"

namespace dash {

/**
 * Merges a partial report into the whole one: objects key by key, arrays of objects with an
 * "id" (AMS units, trays, extruders) item by item, anything else replaced.
 */
void merge_report(nlohmann::json &whole, const nlohmann::json &part);

/** The Printer of web/src/lib/types.ts from the merged "print" report. */
nlohmann::json printer_json(const nlohmann::json &report, const PrinterSettings &settings);

/** Publishes "printer" in the background until destroyed. */
class PrinterProvider {
public:
  PrinterProvider(Hub &hub, PrinterSettings settings);

private:
  std::jthread thread_;
};

} // namespace dash
