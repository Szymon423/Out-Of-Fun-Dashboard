// The state store and the change log that the event streams read.
#include <thread>

#include "check.hpp"
#include "hub.hpp"

using namespace std::chrono_literals;
using nlohmann::json;

int main() {
  dash::Hub hub;

  // Every topic is there from the start, null or empty.
  const json start = hub.snapshot();
  CHECK(start.size() == 8);
  CHECK(start["weather"].is_null());
  CHECK(start["lights"] == json::array());

  std::uint64_t seq = 0;
  CHECK(hub.snapshot_messages(seq).size() == 8);
  CHECK(seq == 0);

  // A change reaches a waiting stream; the same data again does not.
  hub.publish("display", {{"brightness", 40}});
  hub.publish("display", {{"brightness", 40}});
  auto changes = hub.wait(seq, 10ms);
  CHECK(changes.messages.size() == 1);
  CHECK(json::parse(changes.messages[0]) == json({{"topic", "display"}, {"data", {{"brightness", 40}}}}));
  CHECK(hub.snapshot()["display"]["brightness"] == 40);

  // Nothing new: an empty answer after the timeout.
  changes = hub.wait(seq, 10ms);
  CHECK(changes.messages.empty() && !changes.resync && !changes.closed);

  // A stream that missed more than the kept log has to start afresh.
  for (int i = 0; i < 100; ++i) hub.publish("display", {{"brightness", i}});
  changes = hub.wait(seq, 10ms);
  CHECK(changes.resync);

  // close() wakes a waiting stream at once.
  std::thread closer([&] {
    std::this_thread::sleep_for(20ms);
    hub.close();
  });
  const auto before = std::chrono::steady_clock::now();
  changes = hub.wait(seq, 10s);
  closer.join();
  CHECK(changes.closed);
  CHECK(std::chrono::steady_clock::now() - before < 5s);
  return 0;
}
