#pragma once
#include "core/block_tree.hpp"
#include "sim/config.hpp"
#include <set>
namespace bsl::sim {
struct Node {
    std::string id;
    double hash_power;
    BlockTree chain;
    std::set<std::string> first_seen;
    explicit Node(const NodeConfig& config, std::size_t capacity)
        : id(config.id), hash_power(config.hash_power), chain(capacity) {}
};
}
