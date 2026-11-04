#pragma once
#include "sim/event.hpp"
#include <queue>
#include <vector>
#include <stdexcept>
#include <limits>
#include <tuple>
namespace bsl::sim {
class EventQueue {
public:
    explicit EventQueue(std::size_t capacity) : capacity_(capacity) {
        if (!capacity) throw std::invalid_argument("Event capacity must be positive");
    }
    void push(Event event) {
        if (event.time < now_) throw std::invalid_argument("Cannot schedule an event in the past");
        if (queue_.size() >= capacity_) throw std::runtime_error("Pending event limit exceeded");
        if (next_ == std::numeric_limits<std::uint64_t>::max()) throw std::overflow_error("Event sequence exhausted");
        event.sequence = next_++;
        queue_.push(std::move(event));
    }
    bool empty() const { return queue_.empty(); }
    const Event& top() const { if (empty()) throw std::logic_error("Empty event queue"); return queue_.top(); }
    Event pop() {
        const auto event = top();
        queue_.pop();
        now_ = event.time;
        return event;
    }
    Time now() const { return now_; }
    std::size_t size() const { return queue_.size(); }
private:
    struct Later { bool operator()(const Event& a, const Event& b) const {
        return std::tie(a.time, a.sequence) > std::tie(b.time, b.sequence);
    }};
    std::priority_queue<Event, std::vector<Event>, Later> queue_;
    Time now_ = 0;
    std::uint64_t next_ = 0;
    std::size_t capacity_;
};
}
