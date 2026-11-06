#include "attacks/attack.hpp"
#include "sim/simulator.hpp"
#include <algorithm>
namespace bsl::attacks {
void Attack::observe_selfish(sim::Simulator& sim) {
    const auto& public_chain = sim.nodes().at(config_.adversary).chain;
    const auto public_hash = public_chain.tip().hash;
    const bool competition = !last_public_tip.empty() && public_hash != last_public_tip;
    last_public_tip = public_hash;
    if (private_blocks.empty()) return;
    std::uint64_t work = 0;
    auto current = sim.blocks().at(private_blocks.back()).block;
    while (current.index) {
        work += block_work(current.difficulty);
        current = current.previous_hash == genesis_block().hash ? genesis_block() : sim.blocks().at(current.previous_hash).block;
    }
    const auto public_work = public_chain.cumulative_work(public_hash);
    if (work > public_work) max_private_lead = std::max(max_private_lead, (work - public_work) / block_work(sim.config().difficulty));
    if (!competition) return;
    if (work >= public_work) {
        // Publish parent first. A tie becomes a race under the unchanged fork choice.
        const auto release = private_blocks;
        private_blocks.clear();
        for (const auto& hash : release) { sim.publish(config_.adversary, hash); ++publications; }
        last_public_tip = public_chain.tip().hash;
    } else { abandoned += private_blocks.size(); private_blocks.clear(); }
}
bool Attack::selfish_success(const sim::Simulator& sim) const {
    const auto chain = sim.nodes().at(config_.target).chain.active_chain();
    if (chain.size() <= 1) return false;
    std::uint64_t own = 0;
    for (std::size_t i = 1; i < chain.size(); ++i) if (controls(sim.blocks().at(chain[i]).miner)) ++own;
    double total = 0; for (const auto& node : sim.nodes()) total += node.hash_power;
    double adverse=0;for(std::size_t i=0;i<sim.nodes().size();++i)if(controls(i))adverse+=sim.nodes()[i].hash_power;
    return publications > 0 && static_cast<double>(own) / (chain.size() - 1) > adverse / total;
}
}
