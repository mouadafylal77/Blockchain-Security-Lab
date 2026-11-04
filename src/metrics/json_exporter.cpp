#include "metrics/json_exporter.hpp"
#include "metrics/security_export.hpp"
#include "detection/config_io.hpp"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>

namespace bsl::metrics {
using Json = nlohmann::json;
std::string results_json(const sim::Simulator& sim) {
    const auto& m = sim.metrics();
    Json nodes = Json::array(), blocks = Json::array(), trace = Json::array();
    std::vector<std::uint64_t> mined(sim.nodes().size(), 0);
    for (const auto& item : sim.blocks()) {
        const auto& b = item.second;
        ++mined[b.miner];
        blocks.push_back({{"hash", b.block.hash}, {"parent", b.block.previous_hash}, {"height", b.block.index},
            {"miner", sim.nodes()[b.miner].id}, {"discovered_at", b.discovered}, {"difficulty", b.block.difficulty},
            {"nonce", b.block.nonce}, {"transactions", b.block.transactions}, {"merkle_root", b.block.merkle_root}});
    }
    for (std::size_t i = 0; i < sim.nodes().size(); ++i) {
        const auto& n = sim.nodes()[i];
        const auto active = n.chain.active_chain();
        std::uint64_t own_active = 0;
        for (std::size_t j = 1; j < active.size(); ++j) if (sim.blocks().at(active[j]).miner == i) ++own_active;
        nodes.push_back({{"id", n.id}, {"tip", n.chain.tip().hash}, {"height", n.chain.tip().index},
            {"cumulative_work", n.chain.cumulative_work(n.chain.tip().hash)}, {"active_chain", active},
            {"known_blocks_excluding_genesis", n.chain.known_count() - 1}, {"stale_blocks", n.chain.stale_count()},
            {"unknown_parent_blocks", n.chain.pending_count()}, {"blocks_mined", mined[i]}, {"own_active_blocks", own_active}});
    }
    for (const auto& record : sim.trace()) trace.push_back({{"time", record.time}, {"node", sim.nodes()[record.node].id},
        {"type", record.type}, {"block", record.hash}, {"old_tip", record.old_tip}, {"new_tip", record.new_tip},
        {"detached_depth", record.depth}});
    const auto mean = m.propagation_samples ? static_cast<double>(m.propagation_sum_ticks) / static_cast<double>(m.propagation_samples) : 0;
    return Json{{"security",Json::parse(security_json(sim))},{"attack", Json::parse(attacks::results_json(sim))}, {"schema_version", 1}, {"software_version", BSL_VERSION}, {"model", "m2_pow_ticks_v1"},
        {"build", {{"compiler", __VERSION__}, {"cplusplus", __cplusplus}}},
        {"config", Json::parse(sim::config_json(sim.config()))},
        {"execution", {{"stop_reason", sim.stop_reason()}, {"last_event_tick", sim.now()}, {"pending_events", sim.pending_events()}}},
        {"metrics", {{"events_processed", m.events}, {"blocks_produced", m.blocks_produced},
            {"messages_sent", m.messages_sent}, {"messages_delivered", m.messages_delivered}, {"duplicates", m.duplicates},
            {"unknown_parent_arrivals", m.unknown_parent_arrivals}, {"fork_observations", m.forks},
            {"reorganizations", m.reorganizations}, {"propagation_samples", m.propagation_samples},
            {"propagation_mean_ticks", mean}, {"propagation_max_ticks", m.propagation_max_ticks}}},
        {"measurement_definitions", {{"fork_observations", "Per-node second-or-later connected child of one parent"},
            {"reorganizations", "Per-node tip change detaching at least one active block"},
            {"propagation", "First payload arrival at each non-origin node minus discovery time; pending parents included; censored at stop"},
            {"stale_blocks", "Locally connected valid blocks outside active chain"},
            {"blocks_mined", "All produced blocks by origin, including stale; not economic rewards"}}},
        {"nodes", nodes}, {"blocks", blocks}, {"trace", trace}}.dump(2) + "\n";
}
void write_results(const sim::Simulator& simulator, const std::string& output_directory) {
    const std::filesystem::path directory(output_directory);
    if (output_directory.empty()) throw std::invalid_argument("Output directory is empty");
    std::filesystem::create_directories(directory);
    std::ofstream file(directory / "results.json", std::ios::binary | std::ios::trunc);
    if (!file) throw std::runtime_error("Cannot open results.json for writing");
    file << results_json(simulator);
    file.close();
    if (!file) throw std::runtime_error("Cannot write results.json");
    if(simulator.config().security.mode=="calibration"){
        std::ofstream reference(directory / "calibration.json",std::ios::binary | std::ios::trunc);
        auto calibration=Json::parse(detection::reference_json(simulator.config().security,simulator.detector()));
        calibration["source_config"]=Json::parse(sim::config_json(simulator.config()));
        calibration["software_version"]=BSL_VERSION;
        reference << calibration.dump(2) << '\n';reference.close();
        if(!reference)throw std::runtime_error("Cannot write calibration.json");
    }
}
}
