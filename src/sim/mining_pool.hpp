#pragma once
#include "attacks/attack.hpp"
#include <algorithm>
namespace bsl::sim {
class MiningPool {
public:
    MiningPool(std::size_t nodes, const std::vector<std::size_t>& members, bool escrow=false) : shares(nodes, 0), observed_shares(nodes,0), submitted_full(nodes,0), escrowed(nodes,0), released(nodes,0), held_ever(nodes,0), credit_ticks(nodes,0), members_(members), escrow_(escrow) {}
    bool member(std::size_t node) const { return std::find(members_.begin(), members_.end(), node) != members_.end(); }
    void share(std::size_t node,bool credit=true) { if (member(node)) {++observed_shares.at(node);if(credit)++shares.at(node);else {++shares_uncredited;if(escrow_){++escrowed.at(node);++held_ever.at(node);}}} }
    void settle(std::uint64_t now){
        if(!escrow_)return;
        for(std::size_t i=0;i<shares.size();++i)credit_ticks[i]+=escrowed[i]*(now-last_settlement_);
        last_settlement_=now;
    }
    void release(std::size_t node){
        shares.at(node)+=escrowed.at(node);released.at(node)+=escrowed.at(node);escrowed.at(node)=0;
    }
    void full(std::size_t node, bool withheld,bool credit=true) {
        if (!member(node)) return;
        share(node,credit);
        if (withheld) ++full_withheld; else {++full_submitted;++submitted_full.at(node);}
    }
    std::vector<std::uint64_t> shares;
    std::vector<std::uint64_t> observed_shares, submitted_full;
    std::vector<std::uint64_t> escrowed, released, held_ever, credit_ticks;
    std::uint64_t shares_uncredited=0;
    std::uint64_t full_submitted = 0, full_withheld = 0;
private:
    std::vector<std::size_t> members_;
    bool escrow_;
    std::uint64_t last_settlement_=0;
};
}
