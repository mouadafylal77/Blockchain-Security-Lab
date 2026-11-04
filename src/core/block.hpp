#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace bsl {
// Transactions remain opaque teaching strings, without balances or signatures.
struct Block {
    std::uint64_t index = 0;
    std::string previous_hash;
    std::vector<std::string> transactions;
    std::string merkle_root;
    std::uint64_t simulated_time = 0;
    unsigned difficulty = 0; // Number of leading zero hexadecimal digits.
    std::uint64_t nonce = 0;
    std::string hash;
};

constexpr unsigned max_difficulty = 6;
std::string merkle_root(const std::vector<std::string>& transactions);
std::string calculate_hash(const Block& block);
bool has_proof_of_work(const std::string& hash, unsigned difficulty);
bool is_valid_pow_block(const Block& block);
Block genesis_block();
// Recomputes the commitment and searches from nonce zero; throws on exhaustion.
Block mine_block(Block candidate, std::uint64_t max_attempts = 1000000);
}
