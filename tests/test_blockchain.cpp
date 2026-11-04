#include "core/blockchain.hpp"
#include "core/sha256.hpp"

#include <functional>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>

namespace {
void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

template<class Exception, class Function>
void expect_throw(Function function) {
    try { function(); }
    catch (const Exception&) { return; }
    throw std::runtime_error("Expected exception was not raised");
}

bsl::Blockchain sample_chain() {
    bsl::Blockchain chain;
    chain.mine_next({"Alice -> Bob : 10", "Bob -> Charlie : 5", "Charlie -> Alice : 2"}, 10, 1);
    chain.mine_next({}, 20, 1);
    return chain;
}

void test_sha256() {
    require(bsl::sha256("abc") == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad", "SHA-256 abc vector");
    require(bsl::sha256(std::string_view{}) == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855", "SHA-256 empty vector");
    require(bsl::sha256(std::string("a\0b", 3)) != bsl::sha256("a"), "Embedded NUL must be hashed");
}

void test_merkle() {
    const auto a = bsl::sha256("a"), b = bsl::sha256("b"), c = bsl::sha256("c");
    require(bsl::merkle_root({}) == bsl::sha256(""), "Empty Merkle tree");
    require(bsl::merkle_root({"a"}) == a, "Single leaf");
    require(bsl::merkle_root({"a", "b"}) == bsl::sha256(a + b), "Pair");
    require(bsl::merkle_root({"a", "b", "c"}) == bsl::sha256(bsl::sha256(a + b) + bsl::sha256(c + c)), "Odd leaf duplication");
    require(bsl::merkle_root({"a", "b"}) != bsl::merkle_root({"b", "a"}), "Ordered transactions");
    auto block = sample_chain().blocks()[1];
    const auto original_header_hash = bsl::calculate_hash(block);
    block.transactions.push_back(block.transactions.back());
    // Merkle duplication alone cannot distinguish [a,b,c] from [a,b,c,c].
    require(block.merkle_root == bsl::merkle_root(block.transactions), "Duplicate-tail fixture");
    require(bsl::calculate_hash(block) != original_header_hash, "Header must commit to transaction count");
    require(!bsl::is_valid_pow_block(block), "Duplicate transaction tampering");
}

void test_valid_chain() {
    const auto chain = sample_chain();
    require(chain.blocks().size() == 3 && chain.is_valid(), "Valid mined chain");
    require(bsl::Blockchain{}.is_valid(), "Genesis-only chain");
    require(bsl::is_valid_pow_block(chain.blocks()[1]), "Mined block validity");
    require(chain.blocks()[2].transactions.empty(), "Empty transaction block accepted");
    require(!bsl::is_valid_chain({}), "Empty chain is invalid");
}

void test_invalid_parent() {
    auto chain = sample_chain();
    auto broken = chain.blocks();
    broken[1].previous_hash = std::string(64, 'f');
    broken[1] = bsl::mine_block(broken[1]);
    require(bsl::is_valid_pow_block(broken[1]), "Wrong-parent block still has valid PoW");
    require(!bsl::is_valid_chain(broken), "Broken link rejected independently of hash integrity");
    bsl::Blockchain receiver;
    expect_throw<std::invalid_argument>([&] { receiver.append(broken[1]); });
    require(receiver.blocks().size() == 1 && receiver.is_valid(), "Rejected append preserves chain");
    expect_throw<std::invalid_argument>([&] { chain.append(chain.blocks()[1]); });
}

void test_tampering() {
    const auto original = sample_chain().blocks();
    const std::vector<std::function<void(bsl::Block&)>> mutations = {
        [](auto& b) { b.transactions[0] = "Mallory -> Bob : 999"; },
        [](auto& b) { b.merkle_root[0] = b.merkle_root[0] == 'a' ? 'b' : 'a'; },
        [](auto& b) { ++b.index; },
        [](auto& b) { ++b.simulated_time; },
        [](auto& b) { ++b.nonce; },
        [](auto& b) { b.difficulty = 2; },
        [](auto& b) { b.hash[0] = 'f'; }
    };
    for (const auto& mutate : mutations) {
        auto altered = original;
        mutate(altered[1]);
        require(!bsl::is_valid_chain(altered), "Altered block must invalidate chain");
    }
    auto altered = original;
    altered[1].index = 5;
    altered[1] = bsl::mine_block(altered[1]);
    require(!bsl::is_valid_chain(altered), "Re-mining does not repair wrong index");
    altered = original;
    altered[2].simulated_time = 9;
    altered[2] = bsl::mine_block(altered[2]);
    require(!bsl::is_valid_chain(altered), "Re-mined backwards time is invalid");
}

void test_genesis() {
    const auto expected = bsl::genesis_block();
    require(expected.hash == bsl::genesis_block().hash, "Stable genesis");
    const std::vector<std::function<void(bsl::Block&)>> mutations = {
        [](auto& b) { b.index = 1; },
        [](auto& b) { b.previous_hash = "1"; },
        [](auto& b) { b.transactions = {"Different genesis"}; },
        [](auto& b) { b.simulated_time = 1; },
        [](auto& b) { b.difficulty = 1; },
        [](auto& b) { b.nonce = 1; }
    };
    for (const auto& mutate : mutations) {
        auto genesis = expected;
        mutate(genesis);
        genesis.merkle_root = bsl::merkle_root(genesis.transactions);
        genesis.hash = bsl::calculate_hash(genesis);
        require(!bsl::is_valid_chain({genesis}), "Rehashed foreign genesis rejected");
    }
    auto genesis = expected;
    genesis.hash = "";
    require(!bsl::is_valid_chain({genesis}), "Corrupt genesis hash rejected");
}

void test_pow() {
    require(bsl::has_proof_of_work(std::string(64, '0'), 6), "Maximum supported difficulty");
    for (const unsigned difficulty : {0u, 7u, 1000000u}) {
        require(!bsl::has_proof_of_work(std::string(64, '0'), difficulty), "Invalid difficulty");
    }
    require(!bsl::has_proof_of_work("0", 1), "Short hash");
    require(!bsl::has_proof_of_work("0" + std::string(63, 'z'), 1), "Nonhex hash");
    require(!bsl::has_proof_of_work(std::string(64, 'f'), 1), "Insufficient work");
    auto invalid = sample_chain().blocks();
    invalid[1].difficulty = 0;
    invalid[1].hash = bsl::calculate_hash(invalid[1]);
    require(!bsl::is_valid_chain(invalid), "Self-consistent zero-work block rejected");
    auto block = sample_chain().blocks()[1];
    // Find a deterministic header whose nonce zero fails, then give one attempt.
    block.nonce = 0;
    while (bsl::has_proof_of_work(bsl::calculate_hash(block), block.difficulty)) ++block.simulated_time;
    expect_throw<std::runtime_error>([&] { bsl::mine_block(block, 1); });
}

void test_boundaries() {
    auto chain = sample_chain();
    const auto tip = chain.blocks().back().hash;
    for (const unsigned difficulty : {0u, 7u}) {
        expect_throw<std::invalid_argument>([&] { chain.mine_next({}, 30, difficulty); });
    }
    expect_throw<std::invalid_argument>([&] { chain.mine_next({}, 30, 1, 0); });
    expect_throw<std::invalid_argument>([&] { chain.mine_next({}, 19, 1); });
    require(chain.blocks().back().hash == tip && chain.blocks().size() == 3, "Failures preserve chain");
    chain.mine_next({}, 20, 1);
    require(chain.is_valid(), "Equal simulated timestamps permitted");
}

void test_deterministic() {
    const auto a = sample_chain(), b = sample_chain();
    require(a.blocks().size() == b.blocks().size(), "Same chain size");
    for (std::size_t i = 0; i < a.blocks().size(); ++i) {
        require(a.blocks()[i].hash == b.blocks()[i].hash && a.blocks()[i].nonce == b.blocks()[i].nonce,
                "Same inputs yield identical mining results");
    }
}
}

int main(int argc, char* argv[]) {
    const std::map<std::string, std::function<void()>> tests = {
        {"sha256", test_sha256}, {"merkle", test_merkle}, {"valid_chain", test_valid_chain},
        {"invalid_parent", test_invalid_parent}, {"tampering", test_tampering},
        {"genesis", test_genesis}, {"pow", test_pow}, {"boundaries", test_boundaries},
        {"deterministic", test_deterministic}
    };
    try {
        if (argc != 2 || tests.count(argv[1]) == 0) throw std::invalid_argument("Unknown test case");
        tests.at(argv[1])();
        std::cout << "PASS " << argv[1] << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
