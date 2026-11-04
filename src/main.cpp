#include "core/blockchain.hpp"

#include <exception>
#include <iostream>
#include <string_view>

int main(int argc, char* argv[]) {
    if (argc == 2 && std::string_view(argv[1]) == "--help") {
        std::cout << "Usage: blockchain_sim [--demo|--help]\nM1: deterministic linear PoW validation demo.\n";
        return 0;
    }
    if (argc > 2 || (argc == 2 && std::string_view(argv[1]) != "--demo")) {
        std::cerr << "Unknown arguments. Usage: blockchain_sim [--demo|--help]\n";
        return 2;
    }
    try {
        bsl::Blockchain chain;
        chain.mine_next({"Alice -> Bob : 10", "Bob -> Charlie : 5"}, 1);
        chain.mine_next({"Charlie -> Alice : 2"}, 2);
        std::cout << "scenario=m1_demo simulated_time_unit=tick difficulty=2\n";
        for (const auto& block : chain.blocks()) {
            std::cout << "block=" << block.index << " time=" << block.simulated_time
                      << " nonce=" << block.nonce << " hash=" << block.hash << '\n';
        }
        std::cout << "chain_valid=" << (chain.is_valid() ? "true" : "false") << '\n';
        return chain.is_valid() ? 0 : 1;
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}
