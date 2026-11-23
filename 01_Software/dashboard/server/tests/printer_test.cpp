// Turning a Bambu Lab report into the Printer of types.ts, and merging partial reports. The
// report is made up, in the shape a two-nozzle X2D sends (firmware 01.02.00.00).
#include "check.hpp"
#include "printer.hpp"

using nlohmann::json;

namespace {

const dash::PrinterSettings kSettings{"192.168.1.181", "SERIAL", "code", "Drukarka", "X2D"};

json tray(int id, const char *type, const char *color, int remain) {
  return {{"id", std::to_string(id)}, {"tray_type", type}, {"tray_color", color}, {"remain", remain}};
}

json report() {
  return {
      {"gcode_state", "RUNNING"},
      {"subtask_name", "Uchwyt"},
      {"mc_percent", 42},
      {"mc_remaining_time", 35},
      {"layer_num", 60},
      {"total_layer_num", 140},
      {"bed_temper", 64.8},
      {"bed_target_temper", 65.0},
      {"print_error", 0},
      {"hms", json::array()},
      {"device",
       {{"extruder",
         {{"info",
           {{{"id", 0}, {"temp", (220 << 16) | 219}, {"snow", 0x0002}},
            {{"id", 1}, {"temp", 31}, {"snow", 0xFF00}}}}}},
        {"ctc", {{"info", {{"temp", 33}}}}}}},
      {"ams",
       {{"tray_now", "255"},
        {"ams",
         {{{"id", "0"},
           {"humidity_raw", "16"},
           {"tray",
            {tray(0, "PETG", "F95D73FF", -1), tray(1, "PLA", "161616FF", 80), tray(2, "PETG", "FFFFFFFF", 55),
             json{{"id", "3"}}}}}}}}},
  };
}

} // namespace

int main() {
  const json p = dash::printer_json(report(), kSettings);
  CHECK(p["model"] == "X2D");
  CHECK(p["state"] == "printing");
  CHECK(p["job"]["name"] == "Uchwyt");
  CHECK(p["job"]["progress"] == 42);
  CHECK(p["job"]["remainingMin"] == 35);
  CHECK(p["job"]["layer"] == 60 && p["job"]["totalLayers"] == 140);

  // Two nozzles, temperatures unpacked: (target << 16) | current.
  CHECK(p["nozzles"].size() == 2);
  CHECK(p["nozzles"][0] == json({{"temp", 219}, {"target", 220}}));
  CHECK(p["nozzles"][1] == json({{"temp", 31}, {"target", 0}}));
  CHECK(p["bed"] == json({{"temp", 65}, {"target", 65}}));
  CHECK(p["chamber"] == 33);

  // AMS: colours without alpha, unknown remains as null, an empty slot, the slot in use.
  CHECK(p["ams"].size() == 4);
  CHECK(p["ams"][0] == json({{"color", "#F95D73"}, {"material", "PETG"}, {"remaining", nullptr}, {"active", false}}));
  CHECK(p["ams"][1]["remaining"] == 80);
  CHECK(p["ams"][2]["active"] == true); // extruder 0 feeds from unit 0, slot 2
  CHECK(p["ams"][3]["color"].is_null() && p["ams"][3]["material"] == "");
  CHECK(p["amsHumidity"] == 16);
  CHECK(p["message"].is_null());

  // States and messages.
  json r = report();
  r["gcode_state"] = "IDLE";
  CHECK(dash::printer_json(r, kSettings)["state"] == "idle");
  CHECK(dash::printer_json(r, kSettings)["job"].is_null()); // the last job's name stays in the report
  r["gcode_state"] = "FINISH";
  CHECK(dash::printer_json(r, kSettings)["job"]["name"] == "Uchwyt");
  r["gcode_state"] = "FAILED";
  r["print_error"] = 0x0300400C;
  CHECK(dash::printer_json(r, kSettings)["state"] == "error");
  CHECK(dash::printer_json(r, kSettings)["message"] == "Błąd wydruku 0x0300400C");
  r["print_error"] = 0;
  r["hms"] = json::array({{{"attr", 0x03002000}, {"code", 0x00010001}}});
  CHECK(dash::printer_json(r, kSettings)["message"] == "Komunikat drukarki HMS_0300_2000_0001_0001");

  // A single-nozzle printer reports the nozzle at the top.
  json single = report();
  single.erase("device");
  single["nozzle_temper"] = 200.4;
  single["nozzle_target_temper"] = 200;
  CHECK(dash::printer_json(single, kSettings)["nozzles"] == json::array({{{"temp", 200}, {"target", 200}}}));
  CHECK(dash::printer_json(single, kSettings)["chamber"].is_null());

  // Partial updates: a value, one extruder, one tray; the rest stays.
  json whole = report();
  dash::merge_report(whole, {{"mc_percent", 43}});
  dash::merge_report(whole, {{"device", {{"extruder", {{"info", {{{"id", 1}, {"temp", (180 << 16) | 120}}}}}}}}});
  dash::merge_report(whole, {{"ams", {{"ams", {{{"id", "0"}, {"tray", {{{"id", "1"}, {"remain", 79}}}}}}}}}});
  const json merged = dash::printer_json(whole, kSettings);
  CHECK(merged["job"]["progress"] == 43);
  CHECK(merged["nozzles"][0]["temp"] == 219); // untouched
  CHECK(merged["nozzles"][1] == json({{"temp", 120}, {"target", 180}}));
  CHECK(merged["ams"][1]["remaining"] == 79);
  CHECK(merged["ams"][1]["material"] == "PLA"); // the rest of the tray stays
  CHECK(merged["ams"].size() == 4);
  return 0;
}
