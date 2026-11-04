#pragma once
#include "attacks/attack.hpp"
#include <algorithm>
namespace bsl::sim {
class MiningPool {
public:
    MiningPool(std::size_t nodes, const std::vector<std::size_t>& members) : shares(nodes, 0), observed_shares(nodes,0), submitted_full(nodes,0), members_(members) {}
    bool member(std::size_t node) const { return std::find(members_.begin(), members_.end(), node) != members_.end(); }
    void share(std::size_t node,bool credit=true) { if (member(node)) {++observed_shares.at(node);if(credit)++shares.at(node);else ++shares_uncredited;} }
    void full(std::size_t node, bool withheld,bool credit=true) {
        if (!member(node)) return;
        share(node,credit);
        if (withheld) ++full_withheld; else {++full_submitted;++submitted_full.at(node);}
    }
    std::vector<std::uint64_t> shares;
    std::vector<std::uint64_t> observed_shares, submitted_full;
    std::uint64_t shares_uncredited=0;
    std::uint64_t full_submitted = 0, full_withheld = 0;
private:
    std::vector<std::size_t> members_;
};
}
