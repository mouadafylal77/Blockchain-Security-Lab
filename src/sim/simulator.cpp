#include "sim/simulator.hpp"
#include <algorithm>
#include "detection/config_io.hpp"
#include "detection/profile.hpp"

namespace bsl::sim {
namespace {
Config checked(Config config) { validate_config(config); detection::validate_reference_profile(config);return config; }
}
Simulator::Simulator(Config config) : config_(checked(std::move(config))), detector_(config_.security,detection::load_reference(config_.security)),
    observations_(config_.security.window_ticks), defenses_(config_.security), last_arrival_(config_.nodes.size(),0),
    observed_payloads_(config_.nodes.size()), observed_peers_(config_.nodes.size()), observed_producers_(config_.nodes.size()), attack_(config_.attack),
    pool_(config_.nodes.size(), config_.attack.pool_members, config_.security.hardened_security), pool_rng_(config_.seed ^ 0xd1b54a32d192ed03ULL), mining_rng_(config_.seed),
    network_(attacks::network_config(config_)), queue_(static_cast<std::size_t>(config_.max_pending_events)) {
    for (const auto& n : config_.nodes) {
        nodes_.emplace_back(n, static_cast<std::size_t>(config_.max_blocks + 1));
        total_power_ += n.hash_power;
    }
    notice_seen_.assign(nodes_.size(),false);
    reconnection_pending_.assign(nodes_.size(),false);
    diversification_attempted_.assign(nodes_.size(),false);
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
        const auto arrival=queue_.now()+network_.delay(peer);
        queue_.push(Event{arrival, 0, EventType::deliver, peer.node, node, hash});
        trace_.push_back({now(),node,"p2p_send",hash,{}, {},0,"to="+nodes_[peer.node].id+" arrival="+std::to_string(arrival)});
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
    if(config_.security.event_audits && std::any_of(result.updates.begin(),result.updates.end(),[](const auto& u){return u.fork || u.detached_depth;})) security_audit();
}
void Simulator::record_payment(unsigned observed,unsigned required){
    trace_.push_back({now(),config_.attack.target,"payment_accepted",{}, {}, {},observed,
        "required="+std::to_string(required)});
}
void Simulator::observe_merchant_decision(){
    if(config_.attack.type!="51" || attack_.accepted)return;
    const auto node=config_.attack.target;const auto& chain=nodes_[node].chain;
    metrics::MerchantDecision d;d.time=now();d.sequence=current_sequence_;d.tip=chain.tip().hash;
    for(const auto& hash:chain.active_chain()){
        const auto& block=chain.at(hash);
        if(std::find(block.transactions.begin(),block.transactions.end(),"spend:coin:merchant")!=block.transactions.end()){
            d.payment_present=true;d.confirmations=chain.tip().index-block.index+1;break;
        }
    }
    d.required=required_confirmations(node,config_.attack.confirmations);
    d.base_ready=d.payment_present && d.confirmations>=config_.attack.confirmations;
    d.hold_until=defenses_.hold_until(node);d.strengthened_until=defenses_.strengthened_until(node);
    d.hold_active=!defenses_.payment_allowed(node,now());
    d.eligible=d.payment_present && d.confirmations>=d.required && !d.hold_active;
    if(!merchant_decisions_.empty()){
        const auto& p=merchant_decisions_.back();
        if(p.tip==d.tip && p.required==d.required && p.hold_until==d.hold_until && p.strengthened_until==d.strengthened_until
            && p.hold_active==d.hold_active && p.eligible==d.eligible)return;
    }
    merchant_decisions_.push_back(std::move(d));
}
void Simulator::forward_notice(std::size_t node){
    for(const auto& peer:network_.peers(node)){
        queue_.push(Event{now()+network_.delay(peer),0,EventType::payment_notice,peer.node,node,{}});
        ++metrics_.messages_sent;
    }
}
void Simulator::announce_payment(std::size_t node){notice_seen_[node]=true;forward_notice(node);}
std::string Simulator::outcome() const{
    if(config_.attack.type=="none" || !config_.attack.enabled)return "baseline";
    if(attack_.success(*this))return "success";
    if(stop_reason_=="duration_ticks" || stop_reason_=="max_events" || !attack_.private_blocks.empty()
        || (config_.attack.type=="51" && !attack_.accepted))return "inconclusive";
    return "failed";
}
std::string Simulator::outcome_reason() const{
    if(config_.attack.type=="none" || !config_.attack.enabled)return "honest_reference";
    if(attack_.success(*this))return "M3_objective_observed_at_end";
    if(stop_reason_=="duration_ticks" || stop_reason_=="max_events")return "execution_limit_reached";
    if(config_.attack.type=="51" && !attack_.accepted)return "merchant_payment_not_accepted";
    if(!attack_.private_blocks.empty())return "unpublished_private_branch_at_end";
    return "M3_objective_not_observed_in_completed_run";
}
void Simulator::close_production(const std::string& reason){
    if(production_closed_)return;
    production_closed_=true;production_end_tick_=now();production_stop_reason_=reason;
    trace_.push_back({now(),config_.observer,"production_closed",{}, {}, {},0,reason});
}
void Simulator::observe_payment(){
    if(config_.attack.type!="51" || !attack_.accepted)return;
    const auto& chain=nodes_[config_.attack.target].chain;bool active=false;
    for(const auto& hash:chain.active_chain()){
        const auto& tx=chain.at(hash).transactions;
        if(std::find(tx.begin(),tx.end(),"spend:coin:merchant")!=tx.end())active=true;
    }
    if(payment_status_initialized_ && active!=payment_active_)
        trace_.push_back({now(),config_.attack.target,active?"payment_restored":"payment_reversed",{}, {}, {},0});
    payment_status_initialized_=true;payment_active_=active;
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
    if(config_.security.hardened_security && config_.security.mode=="protected" && config_.security.mitigate_peers)
        diversify(config_.attack.target); // Publicly designated protected client, not all peers.
    if(config_.security.mode!="off")queue_.push(Event{config_.security.audit_ticks,0,EventType::security_audit,0,0,{}});
    while (!queue_.empty()) {
        const auto next_type=queue_.top().type;
        if(production_closed_ && (next_type==EventType::mine || next_type==EventType::share || next_type==EventType::security_audit || next_type==EventType::close_production)){
            queue_.discard_top();++cancelled_events_;continue;
        }
        if (queue_.top().time > config_.duration_ticks) { stop_reason_ = "duration_ticks"; break; }
        if (metrics_.events == config_.max_events) { stop_reason_ = "max_events"; break; }
        const auto event = queue_.pop();
        current_sequence_=event.sequence;
        reconcile_pool();
        const auto trace_begin=trace_.size();
        ++metrics_.events;
        if (event.type == EventType::mine) {
            if (!production_closed_ && metrics_.blocks_produced < config_.max_blocks) discover(event.node);
        } else if (event.type == EventType::deliver) {
            trace_.push_back({now(),event.node,"received",event.block_hash,{}, {},0,"sender="+nodes_[event.sender].id});
            observe_arrival(event);
            const auto producer=config_.security.hardened_security?public_producer(blocks_.at(event.block_hash).block):event.sender;
            const auto delay=nodes_[event.node].chain.contains(event.block_hash)?0:defenses_.delivery_delay(event.node,producer,now());
            if(delay){
                queue_.push(Event{now()+delay,0,EventType::held_delivery,event.node,event.sender,event.block_hash});
                ++deferred_messages;deferred_ticks+=delay;
                deferred_deliveries_.push_back({now(),current_sequence_,delay,event.node,event.sender,blocks_.at(event.block_hash).miner,event.block_hash});
                trace_.push_back({now(),event.node,"probation_delay",event.block_hash,{}, {},delay});
            }else receive(event.node,event.block_hash,false);
        }else if(event.type==EventType::held_delivery){
            trace_.push_back({now(),event.node,"delayed_delivery",event.block_hash,{},{},0});
            receive(event.node,event.block_hash,false);
        }
        else if(event.type==EventType::payment_notice){
            ++metrics_.messages_delivered;
            last_arrival_[event.node]=now();
            if(!notice_seen_[event.node]){
                notice_seen_[event.node]=true;
                trace_.push_back({now(),event.node,"payment_notice",{}, {}, {},0});
                if(event.node==config_.attack.adversary)attack_.notified=true;
                forward_notice(event.node);
            }
        }else if(event.type==EventType::peer_reconnect){
            reconnection_pending_[event.node]=false;complete_diversification(event.node);
        }else if(event.type==EventType::close_production){
            close_production("target_height_drained");
        }else if(event.type==EventType::security_audit){
            security_audit();
            if(!production_closed_ && metrics_.blocks_produced<config_.max_blocks && now()+config_.security.audit_ticks<=config_.duration_ticks)
                queue_.push(Event{now()+config_.security.audit_ticks,0,EventType::security_audit,0,0,{}});
        }else if (!production_closed_ && metrics_.blocks_produced < config_.max_blocks) {
            pool_.share(event.node,defenses_.credit_share(event.node,now())); schedule_share(event.node);
            if(config_.security.mode!="off")observations_.record(now(),event.node,"submitted_share",1);
        }
        observe_merchant_decision();
        attack_.observe(*this);
        observe_payment();
        if(!height_reached_ && config_.target_height && nodes_[config_.observer].chain.tip().index>=config_.target_height){
            height_reached_=true;target_tick_=now();
            trace_.push_back({now(),config_.observer,"target_reached",{}, {}, {},config_.target_height});
            if(config_.post_target_ticks)queue_.push(Event{now()+config_.post_target_ticks,0,EventType::close_production,config_.observer,config_.observer,{}});
            else close_production("target_height_drained");
        }
        if(metrics_.blocks_produced>=config_.max_blocks)close_production("max_blocks_drained");
        for(auto i=trace_begin;i<trace_.size();++i){trace_[i].event_sequence=current_sequence_;if(observer_)observer_(trace_[i]);}
    }
    if (stop_reason_.empty()) stop_reason_ = production_closed_?production_stop_reason_:"queue_empty";
    const auto final_begin=trace_.size();
    security_audit(false);
    reconcile_pool();
    for(auto i=final_begin;i<trace_.size();++i){trace_[i].event_sequence=current_sequence_;if(observer_)observer_(trace_[i]);}
}
}
