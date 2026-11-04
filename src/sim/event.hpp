#pragma once
#include <cstdint>
#include <cstddef>
#include <string>
namespace bsl::sim {
using Time = std::uint64_t;
enum class EventType { mine, deliver };
struct Event {
    Time time = 0;
    std::uint64_t sequence = 0; // Assigned only by EventQueue.
    EventType type = EventType::mine;
    std::size_t node = 0, sender = 0;
    std::string block_hash;
};
}
