#include "event_queue.h"

namespace dao {

void EventQueue::push(SpikeEvent e) { heap_.push(e); }

bool EventQueue::empty() const { return heap_.empty(); }

const SpikeEvent& EventQueue::top() const { return heap_.top(); }

void EventQueue::pop() { heap_.pop(); }

size_t EventQueue::size() const { return heap_.size(); }

bool EventQueue::advance_to(uint64_t tick, std::vector<SpikeEvent>& delivered) {
  delivered.clear();
  while (!heap_.empty() && heap_.top().delivery_tick <= tick) {
    delivered.push_back(heap_.top());
    heap_.pop();
  }
  return true;
}

}  // namespace dao
