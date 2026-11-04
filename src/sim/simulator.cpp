#include "sim/simulator.hpp"
#include <algorithm>
#include "detection/config_io.hpp"

namespace bsl::sim {
namespace {
Config checked(Config config) { validate_config(config); return config; }
}
Simulator::Simulator(Config config) : config_(checked(std::move(config))), detector_(config_.security,detection::load_reference(config_.security)),
    observations_(config_.security.window_ticks), defenses_(config_.security), last_arrival_(config_.nodes.size(),0),
    observed_payloads_(config_.nodes.size()), observed_peers_(config_.nodes.size()), attack_(config_.attack),
    pool_(config_.nodes.size(), config_.attack.pool_members), pool_rng_(config_.seed ^ 0xd1b54a32d192ed03ULL), mining_rng_(config_.seed),
    network_(attacks::network_config(config_)), queue_(static_cast<std::size_t>(config_.max_pending_events)) {
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
void Simulator::schedule_share(std::size_t miner) {
    if (config_.attack.type != "withholding" || !pool_.member(miner) || nodes_[miner].hash_power == 0) return;
    const auto wait = pool_rng_.exponential_ticks(config_.mean_block_ticks * total_power_ / nodes_[miner].hash_power / 15);
    queue_.push(Event{queue_.now() + wait, 0, EventType::share, miner, miner, {}});
}
void Simulator::broadcast(std::size_t node, const std::string& hash) {
    for (const auto& peer : network_.peers(node)) {
        if (attack_.drop(node, peer.node)) continue;
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
            if(config_.security.mode!="off")observations_.record(now(),node_index,"fork",1);
            ++metrics_.forks;
            trace_.push_back({queue_.now(), node_index, "fork", update.connected_hash, {}, {}, 0});
        }
        if (update.old_tip != update.new_tip) {
            const bool reorg = update.detached_depth != 0;
            if (reorg) ++metrics_.reorganizations;
            if(reorg && config_.security.mode!="off")observations_.record(now(),node_index,"reorg",static_cast<double>(update.detached_depth));
            trace_.push_back({queue_.now(), node_index, reorg ? "reorganization" : "tip_extension",
                update.connected_hash, update.old_tip, update.new_tip, update.detached_depth});
        }
        broadcast(node_index, update.connected_hash);
    }
}
void Simulator::discover(std::size_t miner) {
    const auto& parent = attack_.parent(*this, miner);
    Block candidate;
    candidate.index = parent.index + 1;
    candidate.previous_hash = parent.hash;
    candidate.simulated_time = queue_.now();
    candidate.difficulty = config_.difficulty;
    candidate.transactions = {"miner:" + nodes_[miner].id, "discovery:" + std::to_string(metrics_.blocks_produced)};
    attack_.transactions(*this, miner, candidate);
    const auto block = mine_block(candidate, config_.mining_attempts);
    if (!blocks_.emplace(block.hash, ProducedBlock{block, miner, queue_.now()}).second)
        throw std::logic_error("Duplicate discovery identity");
    ++metrics_.blocks_produced;
    trace_.push_back({queue_.now(), miner, "discovery", block.hash, {}, {}, 0});
    const bool withheld = attack_.retain(miner, block);
    if (withheld) trace_.push_back({now(), miner, "withheld", block.hash, {}, {}, 0});
    pool_.full(miner, withheld,defenses_.credit_share(miner,now()));
    if(config_.security.mode!="off" && pool_.member(miner)){
        observations_.record(now(),miner,"submitted_share",1);
        if(!withheld)observations_.record(now(),miner,"submitted_full",1);
    }
    if (!withheld) receive(miner, block.hash, true);
    if (metrics_.blocks_produced < config_.max_blocks) schedule_mining(miner);
}
void Simulator::run() {
    if (started_) throw std::logic_error("Simulation instances can run only once");
    started_ = true;
    for (std::size_t i = 0; i < nodes_.size(); ++i) schedule_mining(i);
    for (std::size_t i = 0; i < nodes_.size(); ++i) schedule_share(i);
    if(config_.security.mode!="off")queue_.push(Event{config_.security.audit_ticks,0,EventType::security_audit,0,0,{}});
    while (!queue_.empty()) {
        if (queue_.top().time > config_.duration_ticks) { stop_reason_ = "duration_ticks"; break; }
        if (metrics_.events == config_.max_events) { stop_reason_ = "max_events"; break; }
        const auto event = queue_.pop();
        ++metrics_.events;
        if (event.type == EventType::mine) {
            if (metrics_.blocks_produced < config_.max_blocks) discover(event.node);
        } else if (event.type == EventType::deliver) {
            observe_arrival(event);
            const auto delay=nodes_[event.node].chain.contains(event.block_hash)?0:defenses_.delivery_delay(event.node,event.sender,now());
            if(delay){
                queue_.push(Event{now()+delay,0,EventType::held_delivery,event.node,event.sender,event.block_hash});
                ++deferred_messages;deferred_ticks+=delay;
                trace_.push_back({now(),event.node,"probation_delay",event.block_hash,{}, {},delay});
            }else receive(event.node,event.block_hash,false);
        }else if(event.type==EventType::held_delivery){
            trace_.push_back({now(),event.node,"delayed_delivery",event.block_hash,{},{},0});
            receive(event.node,event.block_hash,false);
        }
        else if(event.type==EventType::security_audit){
            security_audit();
            if(metrics_.blocks_produced<config_.max_blocks && now()+config_.security.audit_ticks<=config_.duration_ticks)
                queue_.push(Event{now()+config_.security.audit_ticks,0,EventType::security_audit,0,0,{}});
        }else if (metrics_.blocks_produced < config_.max_blocks) {
            pool_.share(event.node,defenses_.credit_share(event.node,now())); schedule_share(event.node);
            if(config_.security.mode!="off")observations_.record(now(),event.node,"submitted_share",1);
        }
        attack_.observe(*this);
    }
    if (stop_reason_.empty()) stop_reason_ = metrics_.blocks_produced == config_.max_blocks ? "max_blocks_drained" : "queue_empty";
    security_audit(false);
}
}
