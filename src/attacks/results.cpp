#include "attacks/attack.hpp"
#include "sim/simulator.hpp"
#include <nlohmann/json.hpp>
#include <numeric>
namespace bsl::attacks {
std::string results_json(const sim::Simulator& sim) {
    using Json = nlohmann::json;
    const auto& a = sim.attack();
    const auto& c = sim.config().attack;
    Json result{{"type", c.type}, {"enabled", c.enabled}, {"success", a.success(sim)},
        {"merchant_accepted", a.accepted}, {"accepted_confirmations",a.accepted_confirmations},{"accepted_required",a.accepted_required},{"payment_notice_received",a.notified}, {"merchant_accepted_at",a.accepted_at}, {"retained", a.retained}, {"publications", a.publications},
        {"abandoned", a.abandoned}, {"max_private_lead", a.max_private_lead},
        {"private_blocks_at_stop", a.private_blocks}, {"filtered_messages", a.filtered_messages}};
    if (c.type != "none") {
        const auto active = sim.nodes().at(c.target).chain.active_chain();
        std::uint64_t own = 0;
        for (std::size_t i = 1; i < active.size(); ++i) if (a.controls(sim.blocks().at(active[i]).miner)) ++own;
        result["adversary_active_blocks_at_target"] = own;
        result["active_block_share_at_target"] = active.size() > 1 ? static_cast<double>(own) / (active.size() - 1) : 0;
        result["effective_links"] = Json::parse(sim::config_json(network_config(sim.config()))).at("links");
    }
    if (c.type == "withholding") {
        const auto& pool = sim.pool();
        const auto total = std::accumulate(pool.shares.begin(), pool.shares.end(), std::uint64_t{0});
        const double reward = pool_reward(sim);
        Json payouts = Json::array();
        for (std::size_t i = 0; i < pool.shares.size(); ++i)
            payouts.push_back({{"node", sim.nodes()[i].id}, {"shares", pool.shares[i]},
                {"payout_units", total ? reward * pool.shares[i] / total : 0}});
        result["pool"] = {{"share_difficulty", sim.config().difficulty - 1}, {"full_difficulty", sim.config().difficulty},
            {"observed_shares_by_member",pool.observed_shares},{"submitted_full_by_member",pool.submitted_full},{"shares_uncredited",pool.shares_uncredited},
            {"shares_total", total}, {"full_submitted", pool.full_submitted}, {"full_withheld", pool.full_withheld},
            {"canonical_reward_units_at_target", reward}, {"payouts", payouts}};
    }
    return result.dump();
}
}
