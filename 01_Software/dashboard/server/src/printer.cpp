#include "printer.hpp"

#include <MQTTClient.h>

#include <chrono>
#include <condition_variable>
#include <format>
#include <memory>
#include <mutex>
#include <set>

#include "log.hpp"

namespace dash {

using nlohmann::json;
using namespace std::chrono_literals;

namespace {

constexpr int kReceiveMs = 1000;
constexpr auto kPushAll = 5min; // ask for the whole state again now and then
constexpr auto kTemperatureEvery = 3s;
constexpr auto kFirstRetry = 5s;
constexpr auto kMaxRetry = 2min;

int number(const json &object, const char *key) {
  const auto it = object.find(key);
  if (it == object.end()) return 0;
  if (it->is_number()) return static_cast<int>(std::lround(it->get<double>()));
  if (it->is_string()) {
    try {
      return std::stoi(it->get<std::string>());
    } catch (const std::exception &) {
    }
  }
  return 0;
}

std::string text(const json &object, const char *key) {
  const auto it = object.find(key);
  return it != object.end() && it->is_string() ? it->get<std::string>() : std::string();
}

const json &at_path(const json &object, std::initializer_list<const char *> path) {
  static const json kNull;
  const json *node = &object;
  for (const char *key : path) {
    if (!node->is_object() || !node->contains(key)) return kNull;
    node = &(*node)[key];
  }
  return *node;
}

/** Newer printers (H2D, X2D) pack a temperature as (target << 16) | current. */
json temperature(const json &packed) {
  const auto value = packed.is_number() ? static_cast<std::int64_t>(packed.get<double>()) : 0;
  return {{"temp", value & 0xFFFF}, {"target", (value >> 16) & 0xFFFF}};
}

std::string state_of(const std::string &gcode_state) {
  if (gcode_state == "RUNNING" || gcode_state == "PREPARE" || gcode_state == "SLICING") return "printing";
  if (gcode_state == "PAUSE") return "paused";
  if (gcode_state == "FINISH") return "finished";
  if (gcode_state == "FAILED") return "error";
  return "idle";
}

/** AMS slots in use: tray_now, and on two-nozzle printers each extruder's slot. */
std::set<int> active_trays(const json &report) {
  std::set<int> active;
  if (const int now = number(at_path(report, {"ams"}), "tray_now"); now >= 0 && now < 16) active.insert(now);
  for (const json &extruder : at_path(report, {"device", "extruder", "info"})) {
    const int slot = number(extruder, "snow"); // (unit << 8) | tray; 0xFFxx and 0xFExx mean none
    const int unit = slot >> 8;
    const int tray = slot & 0xFF;
    if (unit < 4 && tray < 4) active.insert(unit * 4 + tray);
  }
  return active;
}

bool sleep_for(std::stop_token stop, std::chrono::seconds duration) {
  std::mutex mutex;
  std::condition_variable_any cv;
  std::unique_lock lock(mutex);
  cv.wait_for(lock, stop, duration, [] { return false; });
  return !stop.stop_requested();
}

struct ClientDeleter {
  void operator()(MQTTClient client) const {
    if (MQTTClient_isConnected(client)) MQTTClient_disconnect(client, 500);
    MQTTClient_destroy(&client);
  }
};
using Client = std::unique_ptr<void, ClientDeleter>;

std::string connect_error(int rc) {
  switch (rc) {
  case 4:
  case 5:
    return "the printer refused the access code (check printer.accessCode)";
  case MQTTCLIENT_FAILURE:
    return "no connection (printer off or unreachable?)";
  default:
    return std::format("MQTT error {}", rc);
  }
}

void run(std::stop_token stop, Hub &hub, const PrinterSettings &settings) {
  const std::string uri = "ssl://" + settings.host + ":8883";
  const std::string report_topic = "device/" + settings.serial + "/report";
  const std::string request_topic = "device/" + settings.serial + "/request";
  const std::string pushall = json{{"pushing", {{"sequence_id", "0"}, {"command", "pushall"}}}}.dump();

  json shown = nullptr; // what the page has
  auto last_publish = std::chrono::steady_clock::now();
  std::chrono::seconds retry = kFirstRetry;
  std::string last_error;

  while (!stop.stop_requested()) {
    MQTTClient raw = nullptr;
    if (MQTTClient_create(&raw, uri.c_str(), "out-of-fun-dashboard", MQTTCLIENT_PERSISTENCE_NONE, nullptr) !=
        MQTTCLIENT_SUCCESS) {
      log_error("printer: cannot create the MQTT client for {}", uri);
      return;
    }
    const Client client(raw);

    MQTTClient_SSLOptions ssl = MQTTClient_SSLOptions_initializer;
    ssl.enableServerCertAuth = 0; // the printer's certificate comes from Bambu's own device CA
    ssl.verify = 0;
    MQTTClient_connectOptions options = MQTTClient_connectOptions_initializer;
    options.keepAliveInterval = 30;
    options.cleansession = 1;
    options.connectTimeout = 10;
    options.username = "bblp";
    options.password = settings.access_code.c_str();
    options.ssl = &ssl;

    const int rc = MQTTClient_connect(raw, &options);
    if (rc != MQTTCLIENT_SUCCESS) {
      const std::string error = connect_error(rc);
      if (error != last_error) log_warning("printer: {}: {}, retrying", settings.host, error);
      last_error = error;
      if (!sleep_for(stop, retry)) return;
      retry = std::min<std::chrono::seconds>(retry * 2, kMaxRetry);
      continue;
    }
    log_info("printer: connected to {} at {}", settings.name, settings.host);
    last_error.clear();
    retry = kFirstRetry;

    MQTTClient_subscribe(raw, report_topic.c_str(), 0);
    auto ask_all = [&] {
      MQTTClient_publish(raw, request_topic.c_str(), static_cast<int>(pushall.size()), pushall.data(), 0, 0, nullptr);
    };
    ask_all();
    auto last_pushall = std::chrono::steady_clock::now();
    json whole = json::object();

    while (!stop.stop_requested()) {
      char *topic = nullptr;
      int topic_len = 0;
      MQTTClient_message *message = nullptr;
      const int received = MQTTClient_receive(raw, &topic, &topic_len, &message, kReceiveMs);
      if (received != MQTTCLIENT_SUCCESS && received != MQTTCLIENT_TOPICNAME_TRUNCATED) break; // connection lost
      if (message) {
        const json payload = json::parse(static_cast<const char *>(message->payload),
                                         static_cast<const char *>(message->payload) + message->payloadlen, nullptr,
                                         false);
        MQTTClient_freeMessage(&message);
        MQTTClient_free(topic);
        if (payload.is_object() && payload.contains("print") && payload["print"].is_object()) {
          merge_report(whole, payload["print"]);
        }
      }

      const auto now = std::chrono::steady_clock::now();
      if (whole.contains("gcode_state")) { // the whole state has arrived
        json next = printer_json(whole, settings);
        if (next != shown) {
          bool important = shown.is_null();
          for (const char *key : {"state", "job", "ams", "amsHumidity", "message"}) {
            if (!important && next[key] != shown[key]) important = true;
          }
          if (important || now - last_publish >= kTemperatureEvery) {
            hub.publish("printer", shown = std::move(next));
            last_publish = now;
          }
        }
      }
      if (now - last_pushall >= kPushAll) {
        ask_all();
        last_pushall = now;
      }
    }

    if (stop.stop_requested()) return;
    log_warning("printer: connection to {} lost, reconnecting", settings.host);
    if (shown.is_object()) {
      shown["state"] = "offline";
      hub.publish("printer", shown);
    }
    if (!sleep_for(stop, retry)) return;
  }
}

} // namespace

void merge_report(json &whole, const json &part) {
  if (!whole.is_object() || !part.is_object()) {
    whole = part;
    return;
  }
  for (const auto &[key, value] : part.items()) {
    json &target = whole[key];
    const bool by_id = value.is_array() && target.is_array() && !value.empty() &&
                       std::all_of(value.begin(), value.end(), [](const json &v) {
                         return v.is_object() && v.contains("id");
                       });
    if (value.is_object() && target.is_object()) {
      merge_report(target, value);
    } else if (by_id) {
      for (const json &item : value) {
        auto same = std::find_if(target.begin(), target.end(), [&](const json &t) {
          return t.is_object() && t.contains("id") && t["id"] == item["id"];
        });
        if (same == target.end()) {
          target.push_back(item);
        } else {
          merge_report(*same, item);
        }
      }
    } else {
      target = value;
    }
  }
}

json printer_json(const json &report, const PrinterSettings &settings) {
  const std::string state = state_of(text(report, "gcode_state"));

  json job = nullptr;
  if (const std::string name = text(report, "subtask_name"); !name.empty() && state != "idle") {
    job = {
        {"name", name},
        {"progress", number(report, "mc_percent")},
        {"remainingMin", number(report, "mc_remaining_time")},
        {"layer", number(report, "layer_num")},
        {"totalLayers", number(report, "total_layer_num")},
        {"thumbnailUrl", nullptr},
    };
  }

  json nozzles = json::array();
  for (const json &extruder : at_path(report, {"device", "extruder", "info"})) {
    nozzles.push_back(temperature(extruder.value("temp", json(0))));
  }
  if (nozzles.empty()) { // single-nozzle printers report it at the top
    nozzles.push_back({{"temp", number(report, "nozzle_temper")}, {"target", number(report, "nozzle_target_temper")}});
  }

  json chamber = nullptr;
  if (const json &ctc = at_path(report, {"device", "ctc", "info", "temp"}); ctc.is_number()) {
    chamber = temperature(ctc)["temp"];
  } else if (report.contains("chamber_temper") && report["chamber_temper"].is_number()) {
    chamber = number(report, "chamber_temper");
  }

  json ams = json::array();
  json humidity = nullptr;
  const std::set<int> active = active_trays(report);
  for (const json &unit : at_path(report, {"ams", "ams"})) {
    const int unit_id = number(unit, "id");
    if (humidity.is_null() && unit.contains("humidity_raw")) humidity = number(unit, "humidity_raw");
    for (const json &tray : unit.value("tray", json::array())) {
      const std::string material = text(tray, "tray_type");
      const std::string color = text(tray, "tray_color");
      const int remain = tray.contains("remain") ? number(tray, "remain") : -1;
      ams.push_back({
          {"color", !material.empty() && color.size() >= 6 ? json("#" + color.substr(0, 6)) : json(nullptr)},
          {"material", material},
          {"remaining", !material.empty() && remain >= 0 ? json(remain) : json(nullptr)},
          {"active", active.contains(unit_id * 4 + number(tray, "id"))},
      });
    }
  }

  json message = nullptr;
  if (const int error = number(report, "print_error"); error != 0) {
    message = std::format("Błąd wydruku 0x{:08X}", static_cast<unsigned>(error));
  } else if (const json &hms = at_path(report, {"hms"}); hms.is_array() && !hms.empty()) {
    const auto attr = static_cast<unsigned>(hms[0].value("attr", 0LL));
    const auto code = static_cast<unsigned>(hms[0].value("code", 0LL));
    message = std::format("Komunikat drukarki HMS_{:04X}_{:04X}_{:04X}_{:04X}", attr >> 16, attr & 0xFFFF, code >> 16,
                          code & 0xFFFF);
  }

  return {
      {"name", settings.name},
      {"model", settings.model},
      {"state", state},
      {"job", job},
      {"nozzles", nozzles},
      {"bed", {{"temp", number(report, "bed_temper")}, {"target", number(report, "bed_target_temper")}}},
      {"chamber", chamber},
      {"ams", ams},
      {"amsHumidity", humidity},
      {"message", message},
  };
}

PrinterProvider::PrinterProvider(Hub &hub, PrinterSettings settings)
    : thread_([&hub, settings = std::move(settings)](std::stop_token stop) { run(stop, hub, settings); }) {}

} // namespace dash
