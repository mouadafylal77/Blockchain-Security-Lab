#include "attacks/attack.hpp"
#include "sim/simulator.hpp"
namespace bsl::attacks {
double pool_reward(const sim::Simulator& sim) {
    if (sim.config().attack.type != "withholding") return 0;
    double reward = 0;
    const auto hashes = sim.nodes().at(sim.config().attack.target).chain.active_chain();
    for (std::size_t i = 1; i < hashes.size(); ++i)
        if (sim.pool().member(sim.blocks().at(hashes[i]).miner)) ++reward;
    return reward;
}
}
