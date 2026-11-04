#pragma once
#include "sim/event.hpp"
#include <vector>
#include <string>
namespace bsl::sim {
struct NodeConfig { std::string id; double hash_power = 0; };
struct LinkConfig { std::size_t from = 0, to = 0; Time latency = 0, jitter = 0; };
struct Config {
    std::string scenario = "baseline";
    std::uint64_t seed = 42;
    Time duration_ticks = 1000;
    std::uint64_t max_events = 100000, max_blocks = 1000, max_pending_events = 100000;
    unsigned difficulty = 1;
    std::uint64_t mining_attempts = 1000000;
    double mean_block_ticks = 100;
    std::vector<NodeConfig> nodes;
    std::vector<LinkConfig> links; // Undirected links.
};
void validate_config(const Config& config);
Config parse_config(const std::string& json_text);
Config load_config(const std::string& path);
std::string config_json(const Config& config);
}
