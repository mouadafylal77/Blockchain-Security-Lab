#pragma once
#include "sim/config.hpp"
#include "sim/rng.hpp"
namespace bsl::sim {
struct Peer { std::size_t node; Time latency, jitter; };
class Network {
public:
    explicit Network(const Config& config);
    const std::vector<Peer>& peers(std::size_t node) const { return peers_.at(node); }
    Time delay(const Peer& peer) { return peer.latency + rng_.uniform_ticks(peer.jitter); }
private:
    std::vector<std::vector<Peer>> peers_;
    Rng rng_;
};
}
