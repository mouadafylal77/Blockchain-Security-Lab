#include "core/blockchain.hpp"

#include <limits>
#include <stdexcept>

namespace bsl {
namespace {
bool follows(const Block& block, const Block& parent) {
    return parent.index != std::numeric_limits<std::uint64_t>::max()
        && block.index == parent.index + 1
        && block.previous_hash == parent.hash
        && block.simulated_time >= parent.simulated_time
        && is_valid_pow_block(block);
}
}

bool is_valid_chain(const std::vector<Block>& blocks) {
    if (blocks.empty()) return false;
    const Block expected = genesis_block();
    const Block& first = blocks.front();
    if (first.index != expected.index || first.previous_hash != expected.previous_hash
        || first.transactions != expected.transactions || first.merkle_root != expected.merkle_root
        || first.simulated_time != expected.simulated_time || first.difficulty != expected.difficulty
        || first.nonce != expected.nonce || first.hash != expected.hash) return false;
    for (std::size_t i = 1; i < blocks.size(); ++i) {
        if (!follows(blocks[i], blocks[i - 1])) return false;
    }
    return true;
}

Blockchain::Blockchain() : blocks_{genesis_block()} {}
bool Blockchain::is_valid() const { return is_valid_chain(blocks_); }

void Blockchain::append(const Block& block) {
    if (!follows(block, blocks_.back())) {
        throw std::invalid_argument("Invalid block, parent link, index or simulated time");
    }
    blocks_.push_back(block);
}

void Blockchain::mine_next(const std::vector<std::string>& transactions,
                           std::uint64_t simulated_time, unsigned difficulty,
                           std::uint64_t max_attempts) {
    const Block& parent = blocks_.back();
    if (simulated_time < parent.simulated_time) throw std::invalid_argument("Simulated time cannot go backwards");
    if (parent.index == std::numeric_limits<std::uint64_t>::max()) throw std::overflow_error("Block index exhausted");
    Block candidate;
    candidate.index = parent.index + 1;
    candidate.previous_hash = parent.hash;
    candidate.transactions = transactions;
    candidate.simulated_time = simulated_time;
    candidate.difficulty = difficulty;
    append(mine_block(candidate, max_attempts));
}
}
