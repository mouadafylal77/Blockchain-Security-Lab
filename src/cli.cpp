#include "cli.hpp"
#include "core/blockchain.hpp"
#include "sim/config.hpp"
#include "sim/simulator.hpp"
#include "metrics/json_exporter.hpp"
#include <iostream>
#include <string>
#include <exception>

namespace bsl {
namespace {
int demo() {
    Blockchain chain;
    chain.mine_next({"Alice -> Bob : 10", "Bob -> Charlie : 5"}, 1);
    chain.mine_next({"Charlie -> Alice : 2"}, 2);
    std::cout << "scenario=m1_demo simulated_time_unit=tick difficulty=2\n";
    for (const auto& block : chain.blocks()) {
        std::cout << "block=" << block.index << " time=" << block.simulated_time
                  << " nonce=" << block.nonce << " hash=" << block.hash << '\n';
    }
    std::cout << "chain_valid=" << (chain.is_valid() ? "true" : "false") << '\n';
    return chain.is_valid() ? 0 : 1;
}
constexpr const char* usage = "Usage: blockchain_sim [--demo|--help] OR --config FILE --output DIRECTORY\n";
}
int run_cli(int argc, char* argv[]) {
    try {
        if (argc == 1 || (argc == 2 && std::string(argv[1]) == "--demo")) return demo();
        if (argc == 2 && std::string(argv[1]) == "--help") {
            std::cout << "Blockchain Security Lab\n" << usage;
            return 0;
        }
        std::string config_path, output_path;
        if (argc == 5) {
            for (int i = 1; i < argc; i += 2) {
                const std::string flag = argv[i];
                if (flag == "--config" && config_path.empty()) config_path = argv[i + 1];
                else if (flag == "--output" && output_path.empty()) output_path = argv[i + 1];
                else { std::cerr << usage; return 2; }
            }
        }
        if (config_path.empty() || output_path.empty()) { std::cerr << usage; return 2; }
        sim::Simulator simulator(sim::load_config(config_path));
        simulator.run();
        metrics::write_results(simulator, output_path);
        const auto& m = simulator.metrics();
        std::cout << "scenario=" << simulator.config().scenario << " seed=" << simulator.config().seed
                  << " blocks=" << m.blocks_produced << " forks=" << m.forks << " reorganizations=" << m.reorganizations
                  << " stop=" << simulator.stop_reason() << '\n';
        std::cout << "Results: " << output_path << "/results.json\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}
}
