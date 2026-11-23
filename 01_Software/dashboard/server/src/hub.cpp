#include "hub.hpp"

namespace dash {

using nlohmann::json;

namespace {

std::string message(const std::string &topic, const json &data) {
  return json{{"topic", topic}, {"data", data}}.dump();
}

} // namespace

Hub::Hub()
    : state_{
          {"weather", nullptr}, {"calendars", nullptr}, {"events", nullptr},
          {"music", nullptr},   {"lights", json::array()}, {"printer", nullptr},
          {"photos", nullptr},  {"display", nullptr},
      } {}

void Hub::publish(const std::string &topic, json data) {
  {
    const std::lock_guard lock(mutex_);
    json &slot = state_[topic];
    if (slot == data) return;
    slot = data;
    messages_.push_back(message(topic, data));
    if (messages_.size() > kKeep) messages_.pop_front();
    ++seq_;
  }
  changed_.notify_all();
}

json Hub::snapshot() const {
  const std::lock_guard lock(mutex_);
  return state_;
}

std::vector<std::string> Hub::snapshot_messages(std::uint64_t &seq) const {
  const std::lock_guard lock(mutex_);
  std::vector<std::string> out;
  for (const auto &[topic, data] : state_.items()) out.push_back(message(topic, data));
  seq = seq_;
  return out;
}

Hub::Changes Hub::wait(std::uint64_t &seq, std::chrono::milliseconds timeout) const {
  std::unique_lock lock(mutex_);
  changed_.wait_for(lock, timeout, [&] { return closed_ || seq_ != seq; });
  Changes changes;
  if (closed_) {
    changes.closed = true;
    return changes;
  }
  const std::uint64_t missed = seq_ - seq;
  if (missed > messages_.size()) {
    changes.resync = true;
  } else {
    changes.messages.assign(messages_.end() - static_cast<std::ptrdiff_t>(missed), messages_.end());
  }
  seq = seq_;
  return changes;
}

void Hub::close() {
  {
    const std::lock_guard lock(mutex_);
    closed_ = true;
  }
  changed_.notify_all();
}

} // namespace dash
