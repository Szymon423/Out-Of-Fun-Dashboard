// The settings sheet's values: checked, applied, kept in the file and read back.
#include <filesystem>
#include <fstream>

#include "check.hpp"
#include "settings.hpp"

using nlohmann::json;

int main() {
  namespace fs = std::filesystem;
  const fs::path dir = fs::temp_directory_path() / "dashboard-settings-test";
  fs::remove_all(dir);
  fs::create_directories(dir);
  const fs::path file = dir / "settings.json";

  // Checks: each field and its limits; unknown fields and wrong types are refused.
  CHECK(dash::check_settings({{"photoIntervalS", 30}}).empty());
  CHECK(!dash::check_settings({{"photoIntervalS", 2}}).empty());
  CHECK(!dash::check_settings({{"photoIntervalS", "30"}}).empty());
  CHECK(dash::check_settings({{"photoOrder", "newest"}}).empty());
  CHECK(dash::check_settings({{"saverAfterS", 0}}).empty());
  CHECK(dash::check_settings({{"saverAfterS", 600}}).empty());
  CHECK(!dash::check_settings({{"saverAfterS", 10}}).empty());
  CHECK(!dash::check_settings({{"photoOrder", "oldest"}}).empty());
  CHECK(dash::check_settings({{"nightFrom", "23:30"}, {"nightTo", "06:00"}, {"nightEnabled", true}}).empty());
  CHECK(!dash::check_settings({{"nightFrom", "24:00"}}).empty());
  CHECK(dash::check_settings({{"hiddenPlaylists", {"spotify:playlist:a"}}}).empty());
  CHECK(!dash::check_settings({{"hiddenPlaylists", {"playlist:a"}}}).empty());
  CHECK(!dash::check_settings({{"colour", "red"}}).empty());
  CHECK(!dash::check_settings(json::object()).empty());

  {
    dash::Hub hub;
    dash::SettingsStore store(hub, file);
    CHECK(store.get() == dash::default_settings());
    CHECK(hub.snapshot()["settings"] == dash::default_settings());

    json seen;
    store.on_change([&](const json &now) { seen = now; });
    CHECK(store.apply({{"photoIntervalS", 60}, {"nightEnabled", true}}).empty());
    CHECK(store.get()["photoIntervalS"] == 60);
    CHECK(seen["nightEnabled"] == true);
    CHECK(hub.snapshot()["settings"]["photoIntervalS"] == 60);

    // A bad change leaves everything as it was, even its valid part.
    CHECK(!store.apply({{"photoIntervalS", 90}, {"photoOrder", "sideways"}}).empty());
    CHECK(store.get()["photoIntervalS"] == 60);
  }

  // Read back from the file; a broken field in it does not lose the others.
  {
    dash::Hub hub;
    dash::SettingsStore store(hub, file);
    CHECK(store.get()["photoIntervalS"] == 60 && store.get()["nightEnabled"] == true);
  }
  std::ofstream(file) << R"({"photoIntervalS": 1, "nightFrom": "21:00"})";
  {
    dash::Hub hub;
    dash::SettingsStore store(hub, file);
    CHECK(store.get()["photoIntervalS"] == 20); // out of range: the default
    CHECK(store.get()["nightFrom"] == "21:00");
  }
  fs::remove_all(dir);
  return 0;
}
