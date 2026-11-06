#include "sim/simulator.hpp"
#include <algorithm>
#include <cmath>
#include <numeric>
namespace bsl::sim {
std::size_t Simulator::public_producer(const Block& b) const{
    if(!b.transactions.empty())for(std::size_t i=0;i<config_.nodes.size();++i)
        if(b.transactions.front()=="miner:"+config_.nodes[i].id)return i;
    return config_.nodes.size(); // Unknown producer is never attributed to the relay.
}
void Simulator::reconcile_pool(){
    if(!config_.security.hardened_security)return;
    pool_.settle(now());
    for(std::size_t i=0;i<nodes_.size();++i)if(pool_.escrowed[i] && defenses_.credit_share(i,now())){
        trace_.push_back({now(),i,"escrow_released",{}, {}, {},pool_.escrowed[i]});pool_.release(i);
    }
}
void Simulator::observe_arrival(const Event& e){
    if(config_.security.mode=="off")return;
    last_arrival_[e.node]=now();
    if(nodes_[e.node].chain.contains(e.block_hash)||!observed_payloads_[e.node].insert(e.block_hash).second)return;
    observed_peers_[e.node].insert(e.sender);
    // Only the public header timestamp is read. No private discovery metadata.
    const auto header_time=blocks_.at(e.block_hash).block.simulated_time;
    observations_.record(now(),e.node,"arrival_age",static_cast<double>(now()-std::min(now(),header_time)),"peer:"+std::to_string(e.sender));
    if(config_.security.hardened_security){
        const auto producer=public_producer(blocks_.at(e.block_hash).block);
        if(producer<nodes_.size()){
            observed_producers_[e.node].insert(producer);
            ++producer_samples_[{e.node,producer}];
            observations_.record(now(),e.node,"producer_age",static_cast<double>(now()-std::min(now(),header_time)),"producer:"+std::to_string(producer));
        }
    }
}
void Simulator::security_audit(bool operational){
    if(config_.security.mode=="off"||now()==last_audit_)return;
    last_audit_=now();++security_audits;
    audit_phases_[now()]=operational?(production_closed_?"drain":"production"):"final_observation";
    std::vector<detection::Feature> features;
    const double span=static_cast<double>(std::min(now(),config_.security.window_ticks));
    auto add=[&](std::size_t node,const std::string& metric,double value,const std::string& component="node"){
        features.push_back({now(),node,metric,value,component});
    };
    for(std::size_t i=0;i<nodes_.size();++i){
        const auto reorg=observations_.values(now(),i,"reorg");
        const auto forks=observations_.values(now(),i,"fork");
        add(i,"reorg_depth",reorg.empty()?0:*std::max_element(reorg.begin(),reorg.end()));
        add(i,"fork_rate",span?forks.size()/span:0);
        const auto& chain=nodes_[i].chain;
        add(i,"stale_ratio",chain.known_count()>1?double(chain.stale_count())/(chain.known_count()-1):0);
        std::map<std::string,unsigned> producers;
        const auto active=chain.active_chain();
        unsigned most=0;
        for(std::size_t j=1;j<active.size();++j){const auto& tx=chain.at(active[j]).transactions;if(!tx.empty())most=std::max(most,++producers[tx.front()]);}
        if(active.size()>1)add(i,"reward_concentration",double(most)/(active.size()-1));
        for(const auto peer:(config_.security.hardened_security?observed_producers_[i]:observed_peers_[i])){
            const auto component=std::string(config_.security.hardened_security?"producer:":"peer:")+std::to_string(peer);
            auto ages=observations_.values(now(),i,config_.security.hardened_security?"producer_age":"arrival_age",component);
            if(!ages.empty()){
                std::sort(ages.begin(),ages.end());
                add(i,"late_p95",ages[static_cast<std::size_t>(std::ceil(ages.size()*0.95))-1],component);
                features.back().evidence_sufficient=ages.size()>=2;
                if(config_.security.hardened_security)features.back().samples=producer_samples_[{i,peer}];
            }
        }
        const auto& peers=network_.peers(i);
        std::map<std::string,unsigned> groups;unsigned largest=0;
        for(const auto& peer:peers){const auto& n=config_.nodes[peer.node];largest=std::max(largest,++groups[n.peer_group.empty()?n.id:n.peer_group]);}
        if(peers.size()>=2)add(i,"peer_concentration",double(largest)/peers.size(),"peers");
        if(!peers.empty())add(i,"silence_ticks",static_cast<double>(now()-last_arrival_[i]),"peers");
        if(pool_.member(i)){
            const auto shares=observations_.values(now(),i,"submitted_share");
            const auto full=observations_.values(now(),i,"submitted_full");
            if(shares.size()>=config_.security.min_shares){
                const double expected=shares.size()/16.0, found=static_cast<double>(full.size());
                add(i,"share_deficit",std::max(0.0,(expected-found)/std::sqrt(std::max(1.0,expected))),"pool_member");
                // Chernoff lower-tail bound for N Bernoulli(1/16) tickets.
                // Overlapping windows are persistence, not independent evidence.
                const double bound=found>=expected?1:std::exp(-expected+(found?found*(1+std::log(expected/found)):0));
                features.back().evidence_sufficient=expected>=8 && (features.back().value<=config_.security.thresholds.at("share_deficit") || bound<=config_.security.pool_tail_probability);
                features.back().samples=pool_.observed_shares[i];
            }
        }
    }
    peak_observation_samples=std::max<std::uint64_t>(peak_observation_samples,observations_.retained_samples());
    const auto alerts=detector_.observe(features);
    for(const auto& alert:alerts)trace_.push_back({now(),alert.node,"alert",{}, {}, {},0,
        alert.metric+"="+std::to_string(alert.value)+" threshold="+std::to_string(alert.threshold)+" phase="+audit_phases_.at(now())});
    if(operational)defenses_.review(features);
    if(operational)for(const auto& alert:alerts){
        const auto previous=defenses_.actions;
        const bool confirmation_signal=alert.metric=="reorg_depth" || alert.metric=="fork_rate" || alert.metric=="silence_ticks";
        // One modeled merchant payment: after its public acceptance, changing its
        // confirmation policy has no operational benefit. Other nodes are not merchants.
        const bool merchant_closed=std::any_of(trace_.begin(),trace_.end(),[&](const auto& e){return e.node==config_.attack.target && e.type=="payment_accepted";});
        if(!config_.security.hardened_security || !confirmation_signal || !config_.security.mitigate_confirmations
            || (alert.node==config_.attack.target && !merchant_closed))defenses_.apply(alert);
        if(previous!=defenses_.actions)trace_.push_back({now(),alert.node,"security_action",{}, {}, {},0,alert.metric});
        if(config_.security.mode=="protected" && config_.security.mitigate_peers && alert.behavior=="connectivity_anomaly"
            && (!config_.security.hardened_security || alert.node==config_.attack.target))diversify(alert.node);
    }
    if(operational)reconcile_pool();
}
void Simulator::diversify(std::size_t node){
    if(reconnection_pending_[node])return;
    if(config_.security.hardened_security && diversification_attempted_[node])return;
    diversification_attempted_[node]=true;
    trace_.push_back({now(),node,"bootstrap_request",{}, {}, {},nodes_[node].chain.tip().index});
    if(config_.security.bootstrap_delay_ticks){
        reconnection_pending_[node]=true;
        queue_.push(Event{now()+config_.security.bootstrap_delay_ticks,0,EventType::peer_reconnect,node,node,{}});
    }else complete_diversification(node);
}
void Simulator::complete_diversification(std::size_t node){
    std::vector<std::size_t> desired;std::map<std::string,unsigned> groups;
    auto include=[&](std::size_t peer){
        const auto& n=config_.nodes[peer];const auto group=n.peer_group.empty()?n.id:n.peer_group;
        if(peer!=node && desired.size()<config_.security.max_peers && groups[group]<config_.security.max_per_group
            && std::find(desired.begin(),desired.end(),peer)==desired.end()){desired.push_back(peer);++groups[group];}
    };
    for(const auto& peer:network_.peers(node))include(peer.node);
    if(config_.security.hardened_security && desired.size()==config_.security.max_peers && desired.size()>1){
        // Reserve the last slot for a public directory candidate from a new group.
        const auto directory=config_.security.restrict_bootstrap?config_.security.bootstrap_peers:[&]{
            std::vector<std::size_t> all;for(std::size_t i=0;i<nodes_.size();++i)all.push_back(i);return all;}();
        for(const auto peer:directory){
            const auto& n=config_.nodes[peer];const auto group=n.peer_group.empty()?n.id:n.peer_group;
            if(peer==node || groups[group] || std::find(desired.begin(),desired.end(),peer)!=desired.end())continue;
            const auto& last=config_.nodes[desired.back()];--groups[last.peer_group.empty()?last.id:last.peer_group];
            desired.pop_back();include(peer);break;
        }
    }
    // Public bootstrap directory; group diversity is not a guarantee of honesty.
    if(config_.security.restrict_bootstrap){for(const auto peer:config_.security.bootstrap_peers)include(peer);}
    else for(std::size_t peer=0;peer<nodes_.size();++peer)include(peer);
    const auto added=network_.set_peers(node,desired);
    for(const auto peer:added){
        ++reconnections;++defenses_.actions;
        for(const auto& endpoints:{std::make_pair(peer,node),std::make_pair(node,peer)}){
            const auto known=nodes_[endpoints.first].chain.active_chain();
            for(std::size_t j=1;j<known.size();++j){
                if(attack_.drop(endpoints.first,endpoints.second))continue;
                queue_.push(Event{now()+1,0,EventType::deliver,endpoints.second,endpoints.first,known[j]});
                ++metrics_.messages_sent;++sync_messages;
            }
        }
        trace_.push_back({now(),node,"peer_reconnection",{},std::to_string(peer),{},0});
        if(config_.security.hardened_security)trace_.push_back({now(),peer,"peer_connection_inbound",{},std::to_string(node),{},0});
    }
}
}
