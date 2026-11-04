#include "sim/network.hpp"
#include <algorithm>
#include <set>
#include <stdexcept>
namespace bsl::sim {
Network::Network(const Config& c) : peers_(c.nodes.size()), rng_(c.seed ^ 0x9e3779b97f4a7c15ULL) {
    for (const auto& link : c.links) {
        peers_.at(link.from).push_back({link.to, link.latency, link.jitter});
        peers_.at(link.to).push_back({link.from, link.latency, link.jitter});
    }
    for (auto& peers : peers_) std::sort(peers.begin(), peers.end(), [](const Peer& a, const Peer& b) { return a.node < b.node; });
}
std::vector<std::size_t> Network::set_peers(std::size_t node,const std::vector<std::size_t>& desired){
    if(node>=peers_.size())throw std::invalid_argument("Invalid reconnection node");
    std::set<std::size_t> unique;
    for(const auto peer:desired)if(peer>=peers_.size()||peer==node||!unique.insert(peer).second)throw std::invalid_argument("Invalid reconnection peer");
    const auto old=peers_[node];
    for(const auto& peer:old)if(!unique.count(peer.node)){
        auto& reverse=peers_[peer.node];
        reverse.erase(std::remove_if(reverse.begin(),reverse.end(),[&](const auto& p){return p.node==node;}),reverse.end());
    }
    std::vector<Peer> updated;std::vector<std::size_t> added;
    for(const auto peer:desired){
        const auto found=std::find_if(old.begin(),old.end(),[&](const auto& p){return p.node==peer;});
        if(found!=old.end())updated.push_back(*found);
        else{updated.push_back({peer,1,0});peers_[peer].push_back({node,1,0});added.push_back(peer);}
    }
    peers_[node]=std::move(updated);
    for(auto& peers:peers_)std::sort(peers.begin(),peers.end(),[](const auto& a,const auto& b){return a.node<b.node;});
    return added;
}
}
