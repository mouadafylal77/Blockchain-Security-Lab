#include "attacks/attack.hpp"
#include "sim/simulator.hpp"
#include <algorithm>
namespace bsl::attacks {
namespace {
bool contains_spend(const sim::Simulator& sim, const Block& tip, const std::string& spend) {
    Block b = tip;
    while (b.index) {
        if (std::find(b.transactions.begin(), b.transactions.end(), spend) != b.transactions.end()) return true;
        b = b.previous_hash == genesis_block().hash ? genesis_block() : sim.blocks().at(b.previous_hash).block;
    }
    return false;
}
}
const Block& Attack::parent(const sim::Simulator& sim, std::size_t miner) const {
    if (config_.enabled && ((config_.type == "51" && !published) || config_.type == "selfish") && miner == config_.adversary) {
        if (!private_blocks.empty()) return sim.blocks().at(private_blocks.back()).block;
        if (config_.type == "51") { static const Block genesis = genesis_block(); return genesis; }
    }
    return sim.nodes().at(miner).chain.tip();
}
void Attack::transactions(const sim::Simulator& sim, std::size_t miner, Block& b) const {
    if (config_.type != "51") return;
    const auto& p = parent(sim, miner);
    if (!contains_spend(sim, p, "spend:coin:merchant") && !contains_spend(sim, p, "spend:coin:attacker"))
        b.transactions.push_back(config_.enabled && miner == config_.adversary && !published ? "spend:coin:attacker" : "spend:coin:merchant");
}
bool Attack::retain(std::size_t miner, const Block& block) {
    if (!config_.enabled) return false;
    if (config_.type == "withholding" && miner == config_.adversary) { ++retained; return true; }
    if ((config_.type != "51" && config_.type != "selfish") || miner != config_.adversary || (config_.type == "51" && published)) return false;
    private_blocks.push_back(block.hash); ++retained;
    return true;
}
void Attack::observe(sim::Simulator& sim) {
    if (!config_.enabled && config_.type != "51") return;
    if (config_.type == "selfish") { observe_selfish(sim); return; }
    if (config_.type != "51") return;
    const auto& chain = sim.nodes().at(config_.target).chain;
    if(!accepted) for (const auto& hash : chain.active_chain()) {
        const auto& b = chain.at(hash);
        if (std::find(b.transactions.begin(), b.transactions.end(), "spend:coin:merchant") != b.transactions.end()
            && chain.tip().index - b.index + 1 >= sim.required_confirmations(config_.target,config_.confirmations)
            && sim.payment_allowed(config_.target)) { accepted = true; accepted_at = sim.now(); }
    }
    if (config_.enabled && !published && accepted && !private_blocks.empty()) {
        std::uint64_t private_work = 0;
        for (const auto& hash : private_blocks) private_work += block_work(sim.blocks().at(hash).block.difficulty);
        if (private_work > chain.cumulative_work(chain.tip().hash)) {
            published = true;
            for (const auto& hash : private_blocks) { sim.publish(config_.adversary, hash); ++publications; }
            private_blocks.clear();
        }
    }
}
bool Attack::success(const sim::Simulator& sim) const {
    if (!config_.enabled) return false;
    if (config_.type == "sybil") return filtered_messages > 0 && sim.nodes().at(config_.target).chain.known_count() == 1;
    if (config_.type == "withholding") return sim.pool().full_withheld > 0;
    if (config_.type == "selfish") return selfish_success(sim);
    if (config_.type != "51") return false;
    const auto& tip = sim.nodes().at(config_.target).chain.tip();
    return accepted && published && contains_spend(sim, tip, "spend:coin:attacker")
        && !contains_spend(sim, tip, "spend:coin:merchant");
}
}
