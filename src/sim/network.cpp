#include "sim/network.hpp"
#include <algorithm>
namespace bsl::sim {
Network::Network(const Config& c) : peers_(c.nodes.size()), rng_(c.seed ^ 0x9e3779b97f4a7c15ULL) {
    for (const auto& link : c.links) {
        peers_.at(link.from).push_back({link.to, link.latency, link.jitter});
        peers_.at(link.to).push_back({link.from, link.latency, link.jitter});
    }
    for (auto& peers : peers_) std::sort(peers.begin(), peers.end(), [](const Peer& a, const Peer& b) { return a.node < b.node; });
}
}
