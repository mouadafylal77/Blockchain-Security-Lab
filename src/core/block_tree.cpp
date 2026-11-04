#include "core/block_tree.hpp"
#include "core/blockchain.hpp"
#include <limits>
#include <stdexcept>
#include <algorithm>

namespace bsl {
std::uint64_t block_work(unsigned difficulty) {
    if (difficulty < 1 || difficulty > max_difficulty) throw std::invalid_argument("Invalid work difficulty");
    return std::uint64_t{1} << (4 * difficulty);
}
BlockTree::BlockTree(std::size_t capacity) : capacity_(capacity) {
    if (capacity == 0) throw std::invalid_argument("Block tree capacity must be positive");
    const auto genesis = genesis_block();
    tip_ = genesis.hash;
    entries_.emplace(genesis.hash, Entry{genesis, 0, 0});
}
const Block& BlockTree::tip() const { return at(tip_); }
const Block& BlockTree::at(const std::string& hash) const { return entries_.at(hash).block; }
std::uint64_t BlockTree::cumulative_work(const std::string& hash) const { return entries_.at(hash).work; }
bool BlockTree::follows_parent(const Block& block) const {
    const auto& parent = at(block.previous_hash);
    return parent.index != std::numeric_limits<std::uint64_t>::max()
        && block.index == parent.index + 1 && block.simulated_time >= parent.simulated_time;
}
std::uint64_t BlockTree::detached_depth(std::string old_tip, std::string new_tip) const {
    std::uint64_t detached = 0;
    while (at(new_tip).index > at(old_tip).index) new_tip = at(new_tip).previous_hash;
    while (at(old_tip).index > at(new_tip).index) { old_tip = at(old_tip).previous_hash; ++detached; }
    while (old_tip != new_tip) {
        old_tip = at(old_tip).previous_hash;
        new_tip = at(new_tip).previous_hash;
        ++detached;
    }
    return detached;
}
ChainUpdate BlockTree::connect(const Block& block) {
    auto& parent = entries_.at(block.previous_hash);
    const auto work = block_work(block.difficulty);
    if (parent.work > std::numeric_limits<std::uint64_t>::max() - work) throw std::overflow_error("Cumulative work overflow");
    const auto total = parent.work + work;
    ChainUpdate update{block.hash, parent.children != 0, tip_, tip_, 0};
    ++parent.children;
    entries_.emplace(block.hash, Entry{block, total, 0});
    const auto best_work = cumulative_work(tip_);
    if (total > best_work || (total == best_work && block.hash < tip_)) {
        update.detached_depth = detached_depth(tip_, block.hash);
        tip_ = block.hash;
        update.new_tip = tip_;
    }
    return update;
}
ReceiveResult BlockTree::receive(const Block& block) {
    // Validate before duplicate lookup: a forged body reusing a hash is not valid.
    if (block.index == 0) {
        return {is_valid_chain({block}) ? ReceiveStatus::duplicate : ReceiveStatus::invalid, {}};
    }
    if (!is_valid_pow_block(block)) return {ReceiveStatus::invalid, {}};
    if (entries_.count(block.hash) || pending_.count(block.hash)) return {ReceiveStatus::duplicate, {}};
    if (entries_.size() + pending_.size() >= capacity_) throw std::runtime_error("Local block tree capacity exhausted");
    if (!contains(block.previous_hash)) {
        pending_.emplace(block.hash, block);
        return {ReceiveStatus::pending_parent, {}};
    }
    if (!follows_parent(block)) return {ReceiveStatus::invalid, {}};
    ReceiveResult result{ReceiveStatus::accepted, {connect(block)}};
    // Map order gives deterministic attachment even with several waiting children.
    bool progress = true;
    while (progress) {
        progress = false;
        for (auto it = pending_.begin(); it != pending_.end();) {
            if (!contains(it->second.previous_hash)) { ++it; continue; }
            if (follows_parent(it->second)) result.updates.push_back(connect(it->second));
            it = pending_.erase(it);
            progress = true;
        }
    }
    return result;
}
std::vector<std::string> BlockTree::active_chain() const {
    std::vector<std::string> result;
    auto current = tip_;
    for (;;) {
        result.push_back(current);
        if (at(current).index == 0) break;
        current = at(current).previous_hash;
    }
    std::reverse(result.begin(), result.end());
    return result;
}
}
