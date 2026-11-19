#pragma once

#include <cstdint>
#include <queue>
#include <vector>

namespace dao {

struct SpikeEvent {
  uint64_t delivery_tick = 0;
  uint32_t target = 0;
  float amp = 0.0f;
};

class EventQueue {
 public:
  void push(SpikeEvent e);
  bool empty() const;
  const SpikeEvent& top() const;
  void pop();
  size_t size() const;

  // Pop all events with delivery_tick <= tick into delivered.
  bool advance_to(uint64_t tick, std::vector<SpikeEvent>& delivered);

 private:
  struct Cmp {
    bool operator()(const SpikeEvent& a, const SpikeEvent& b) const {
      if (a.delivery_tick != b.delivery_tick) {
        return a.delivery_tick > b.delivery_tick;  // min-heap by tick
      }
      return a.target > b.target;
    }
  };

  std::priority_queue<SpikeEvent, std::vector<SpikeEvent>, Cmp> heap_;
};

}  // namespace dao
