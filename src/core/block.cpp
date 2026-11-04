#include "core/block.hpp"
#include "core/sha256.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace bsl {
namespace {
// Length prefixes prevent ambiguous headers when string fields contain delimiters.
void append_field(std::string& header, const std::string& value) {
    header += std::to_string(value.size()) + ":" + value;
}
}

std::string merkle_root(const std::vector<std::string>& transactions) {
    if (transactions.empty()) return sha256("");
    std::vector<std::string> level;
    for (const auto& tx : transactions) level.push_back(sha256(tx));
    while (level.size() > 1) {
        if (level.size() % 2 != 0) level.push_back(level.back());
        std::vector<std::string> next;
        for (std::size_t i = 0; i < level.size(); i += 2) {
            next.push_back(sha256(level[i] + level[i + 1]));
        }
        level = std::move(next);
    }
    return level.front();
}

std::string calculate_hash(const Block& block) {
    std::string header;
    for (const auto& field : {std::string("BSL-PoW-v1"), std::to_string(block.index),
            block.previous_hash, block.merkle_root, std::to_string(block.transactions.size()),
            std::to_string(block.simulated_time), std::to_string(block.difficulty),
            std::to_string(block.nonce)}) {
        append_field(header, field);
    }
    return sha256(header);
}

bool has_proof_of_work(const std::string& hash, unsigned difficulty) {
    if (difficulty < 1 || difficulty > max_difficulty || hash.size() != 64) return false;
    const bool hex = std::all_of(hash.begin(), hash.end(), [](char c) {
        return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
    });
    return hex && hash.compare(0, difficulty, std::string(difficulty, '0')) == 0;
}

bool is_valid_pow_block(const Block& block) {
    return has_proof_of_work(block.hash, block.difficulty)
        && block.merkle_root == merkle_root(block.transactions)
        && block.hash == calculate_hash(block);
}

Block genesis_block() {
    Block genesis;
    genesis.previous_hash = "0";
    genesis.transactions = {"Bloc genesis"};
    genesis.merkle_root = merkle_root(genesis.transactions);
    genesis.hash = calculate_hash(genesis);
    return genesis;
}

Block mine_block(Block candidate, std::uint64_t max_attempts) {
    if (candidate.difficulty < 1 || candidate.difficulty > max_difficulty) {
        throw std::invalid_argument("PoW difficulty must be between 1 and 6");
    }
    if (max_attempts == 0) throw std::invalid_argument("Mining requires a positive attempt budget");
    candidate.merkle_root = merkle_root(candidate.transactions);
    for (std::uint64_t attempt = 0; attempt < max_attempts; ++attempt) {
        candidate.nonce = attempt;
        candidate.hash = calculate_hash(candidate);
        if (has_proof_of_work(candidate.hash, candidate.difficulty)) return candidate;
    }
    throw std::runtime_error("PoW attempt budget exhausted");
}
}
