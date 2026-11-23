// The dashboard state and its changes. Providers publish a topic ("weather", "lights", ...)
// with its new data; GET /api/state reads the whole state and every GET /api/events stream
// waits here for the next changes. Field names follow web/src/lib/types.ts.
#pragma once

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <mutex>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace dash {

class Hub {
public:
  Hub();

  /** Stores the topic's data and wakes the event streams; unchanged data is ignored. */
  void publish(const std::string &topic, nlohmann::json data);

  /** The whole state: every topic of DashboardState, null where nothing is known yet. */
  nlohmann::json snapshot() const;

  /** Every topic as one update message each, for a stream that has to start afresh. */
  std::vector<std::string> snapshot_messages(std::uint64_t &seq) const;

  struct Changes {
    std::vector<std::string> messages; // JSON of {"topic": ..., "data": ...}
    bool resync = false;               // missed too much: send the snapshot again
    bool closed = false;               // the server is stopping
  };

  /** The messages after `seq` (advanced past them), or none after `timeout`. */
  Changes wait(std::uint64_t &seq, std::chrono::milliseconds timeout) const;

  /** Wakes all waiting streams so they can end; called when the server stops. */
  void close();

private:
  static constexpr std::size_t kKeep = 64; // messages kept for streams that fall behind

  mutable std::mutex mutex_;
  mutable std::condition_variable changed_;
  nlohmann::json state_;
  std::deque<std::string> messages_; // the last kKeep messages; the newest has number seq_
  std::uint64_t seq_ = 0;
  bool closed_ = false;
};

} // namespace dash
