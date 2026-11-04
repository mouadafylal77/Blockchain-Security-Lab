#include "sim/simulator.hpp"
#include <algorithm>

namespace bsl::sim {
namespace {
Config checked(Config config) { validate_config(config); return config; }
}
Simulator::Simulator(Config config) : config_(checked(std::move(config))), mining_rng_(config_.seed),
    network_(config_), queue_(static_cast<std::size_t>(config_.max_pending_events)) {
    for (const auto& n : config_.nodes) {
        nodes_.emplace_back(n, static_cast<std::size_t>(config_.max_blocks + 1));
        total_power_ += n.hash_power;
    }
}
void Simulator::schedule_mining(std::size_t miner) {
    const double share = nodes_.at(miner).hash_power / total_power_;
    if (share == 0) return;
    const auto wait = mining_rng_.exponential_ticks(config_.mean_block_ticks / share);
    queue_.push(Event{queue_.now() + wait, 0, EventType::mine, miner, miner, {}});
}
void Simulator::broadcast(std::size_t node, const std::string& hash) {
    for (const auto& peer : network_.peers(node)) {
        queue_.push(Event{queue_.now() + network_.delay(peer), 0, EventType::deliver, peer.node, node, hash});
        ++metrics_.messages_sent;
    }
}
void Simulator::receive(std::size_t node_index, const std::string& hash, bool local) {
    auto& node = nodes_.at(node_index);
    const auto& produced = blocks_.at(hash);
    const auto result = node.chain.receive(produced.block);
    if (!local) {
        ++metrics_.messages_delivered;
        if (node.first_seen.insert(hash).second && node_index != produced.miner) {
            const auto delay = queue_.now() - produced.discovered;
            ++metrics_.propagation_samples;
            metrics_.propagation_sum_ticks += delay;
            metrics_.propagation_max_ticks = std::max(metrics_.propagation_max_ticks, delay);
        }
    } else node.first_seen.insert(hash);
    if (result.status == ReceiveStatus::invalid) throw std::runtime_error("Simulator generated invalid block");
    if (result.status == ReceiveStatus::duplicate) ++metrics_.duplicates;
    if (result.status == ReceiveStatus::pending_parent) ++metrics_.unknown_parent_arrivals;
    for (const auto& update : result.updates) {
        if (update.fork) {
            ++metrics_.forks;
            trace_.push_back({queue_.now(), node_index, "fork", update.connected_hash, {}, {}, 0});
        }
        if (update.old_tip != update.new_tip) {
            const bool reorg = update.detached_depth != 0;
            if (reorg) ++metrics_.reorganizations;
            trace_.push_back({queue_.now(), node_index, reorg ? "reorganization" : "tip_extension",
                update.connected_hash, update.old_tip, update.new_tip, update.detached_depth});
        }
        broadcast(node_index, update.connected_hash);
    }
}
void Simulator::discover(std::size_t miner) {
    const auto& parent = nodes_.at(miner).chain.tip();
    Block candidate;
    candidate.index = parent.index + 1;
    candidate.previous_hash = parent.hash;
    candidate.simulated_time = queue_.now();
    candidate.difficulty = config_.difficulty;
    candidate.transactions = {"miner:" + nodes_[miner].id, "discovery:" + std::to_string(metrics_.blocks_produced)};
    const auto block = mine_block(candidate, config_.mining_attempts);
    if (!blocks_.emplace(block.hash, ProducedBlock{block, miner, queue_.now()}).second)
        throw std::logic_error("Duplicate discovery identity");
    ++metrics_.blocks_produced;
    trace_.push_back({queue_.now(), miner, "discovery", block.hash, {}, {}, 0});
    receive(miner, block.hash, true);
    if (metrics_.blocks_produced < config_.max_blocks) schedule_mining(miner);
}
void Simulator::run() {
    if (started_) throw std::logic_error("Simulation instances can run only once");
    started_ = true;
    for (std::size_t i = 0; i < nodes_.size(); ++i) schedule_mining(i);
    while (!queue_.empty()) {
        if (queue_.top().time > config_.duration_ticks) { stop_reason_ = "duration_ticks"; break; }
        if (metrics_.events == config_.max_events) { stop_reason_ = "max_events"; break; }
        const auto event = queue_.pop();
        ++metrics_.events;
        if (event.type == EventType::mine) {
            if (metrics_.blocks_produced < config_.max_blocks) discover(event.node);
        } else receive(event.node, event.block_hash, false);
    }
    if (stop_reason_.empty()) stop_reason_ = metrics_.blocks_produced == config_.max_blocks ? "max_blocks_drained" : "queue_empty";
}
}
