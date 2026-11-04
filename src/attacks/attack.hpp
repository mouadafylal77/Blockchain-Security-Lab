#pragma once
#include "core/block.hpp"
#include <cstddef>
namespace bsl::sim { class Simulator; struct Config; }
namespace bsl::attacks {
struct Config {
    std::string type = "none";
    std::size_t adversary = 0, target = 1;
    unsigned confirmations = 2;
    bool enabled = true;
    std::vector<std::size_t> pool_members;
    std::vector<std::size_t> sybil_identities;
    bool capture_connections = true;
};
Config parse_config(const std::string& text);
std::string config_json(const Config& config);
void validate(const sim::Config& config);
double pool_reward(const sim::Simulator& sim);
std::string results_json(const sim::Simulator& sim);
sim::Config network_config(const sim::Config& config);
class Attack {
public:
    explicit Attack(Config config) : config_(config) {}
    const Block& parent(const sim::Simulator& sim, std::size_t miner) const;
    void transactions(const sim::Simulator& sim, std::size_t miner, Block& block) const;
    bool retain(std::size_t miner, const Block& block);
    void observe(sim::Simulator& sim);
    bool success(const sim::Simulator& sim) const;
    bool accepted = false, published = false;
    std::uint64_t accepted_at = 0;
    std::uint64_t retained = 0, publications = 0;
    std::uint64_t abandoned = 0, max_private_lead = 0;
    std::uint64_t filtered_messages = 0;
    bool drop(std::size_t sender, std::size_t recipient);
    std::vector<std::string> private_blocks;
private:
    Config config_;
    std::string last_public_tip;
    void observe_selfish(sim::Simulator& sim);
    bool selfish_success(const sim::Simulator& sim) const;
};
}
