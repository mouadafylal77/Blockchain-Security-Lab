#include "sim/simulator.hpp"
#include <algorithm>
#include <cmath>
#include <numeric>
namespace bsl::sim {
void Simulator::observe_arrival(const Event& e){
    if(config_.security.mode=="off")return;
    last_arrival_[e.node]=now();
    if(nodes_[e.node].chain.contains(e.block_hash)||!observed_payloads_[e.node].insert(e.block_hash).second)return;
    observed_peers_[e.node].insert(e.sender);
    // Only the public header timestamp is read. No private discovery metadata.
    const auto header_time=blocks_.at(e.block_hash).block.simulated_time;
    observations_.record(now(),e.node,"arrival_age",static_cast<double>(now()-std::min(now(),header_time)),"peer:"+std::to_string(e.sender));
}
void Simulator::security_audit(bool operational){
    if(config_.security.mode=="off"||now()==last_audit_)return;
    last_audit_=now();++security_audits;
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
        for(const auto peer:observed_peers_[i]){
            const auto component="peer:"+std::to_string(peer);
            auto ages=observations_.values(now(),i,"arrival_age",component);
            if(!ages.empty()){
                std::sort(ages.begin(),ages.end());
                add(i,"late_p95",ages[static_cast<std::size_t>(std::ceil(ages.size()*0.95))-1],component);
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
            if(shares.size()>=config_.security.min_shares){const double expected=shares.size()/16.0;add(i,"share_deficit",std::max(0.0,(expected-full.size())/std::sqrt(std::max(1.0,expected))),"pool_member");}
        }
    }
    peak_observation_samples=std::max<std::uint64_t>(peak_observation_samples,observations_.retained_samples());
    const auto alerts=detector_.observe(features);
    if(operational)for(const auto& alert:alerts){
        defenses_.apply(alert);
        if(config_.security.mode=="protected" && config_.security.mitigate_peers && alert.behavior=="connectivity_anomaly")diversify(alert.node);
    }
}
void Simulator::diversify(std::size_t node){
    std::vector<std::size_t> desired;std::map<std::string,unsigned> groups;
    auto include=[&](std::size_t peer){
        const auto& n=config_.nodes[peer];const auto group=n.peer_group.empty()?n.id:n.peer_group;
        if(peer!=node && desired.size()<config_.security.max_peers && groups[group]<config_.security.max_per_group
            && std::find(desired.begin(),desired.end(),peer)==desired.end()){desired.push_back(peer);++groups[group];}
    };
    for(const auto& peer:network_.peers(node))include(peer.node);
    // Public bootstrap directory; group diversity is not a guarantee of honesty.
    for(std::size_t peer=0;peer<nodes_.size();++peer)include(peer);
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
    }
}
}
