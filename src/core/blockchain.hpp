#pragma once

#include "core/block.hpp"

namespace bsl {
bool is_valid_chain(const std::vector<Block>& blocks);

// A linear chain only. Block trees and cumulative-work fork choice belong to M2.
class Blockchain {
public:
    Blockchain();
    const std::vector<Block>& blocks() const noexcept { return blocks_; }
    bool is_valid() const;
    void append(const Block& block);
    void mine_next(const std::vector<std::string>& transactions,
                   std::uint64_t simulated_time, unsigned difficulty = 2,
                   std::uint64_t max_attempts = 1000000);
private:
    std::vector<Block> blocks_;
};
}
