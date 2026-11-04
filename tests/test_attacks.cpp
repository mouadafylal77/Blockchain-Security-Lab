#include "sim/simulator.hpp"
#include "metrics/json_exporter.hpp"
#include <iostream>
#include <stdexcept>
using namespace bsl;
void check(bool x, const char* message) { if (!x) throw std::runtime_error(message); }
int main(int argc, char** argv) {
    try {
        if (argc != 2) throw std::runtime_error("Expected attack name");
        const std::string name = argv[1];
        auto c = sim::load_config(std::string(BSL_SOURCE_DIR) + "/configs/" + name + ".json");
        sim::Simulator s(c); s.run();
        if (name == "attack_51") {
            check(s.attack().accepted && s.attack().publications > 0 && s.attack().success(s), "Confirmed payment must be reversed by published private branch");
            check(s.metrics().reorganizations > 0, "Actual reorganization required");
            c.nodes[c.attack.adversary].hash_power = 0;
            sim::Simulator failed(c); failed.run();
            check(failed.attack().accepted && !failed.attack().success(failed) && failed.attack().retained == 0, "Zero-power attack fails with confirmed public payment");
            c = s.config(); c.attack.confirmations = 10000;
            sim::Simulator unconfirmed(c); unconfirmed.run();
            check(!unconfirmed.attack().accepted && unconfirmed.attack().retained > 0 && !unconfirmed.attack().published, "Private mining alone cannot reverse an accepted payment");
        }
        if (name == "selfish_mining") {
            check(s.attack().retained > 0 && s.attack().publications > 0 && s.attack().max_private_lead > 0, "Private lead and strategic publication required");
            check(s.attack().success(s), "Favorable selfish scenario must exceed its configured hash-power share");
            c.nodes[c.attack.adversary].hash_power = 0;
            sim::Simulator failed(c); failed.run();
            check(!failed.attack().success(failed) && failed.attack().publications == 0, "Selfish mining can fail");
        }
        if (name == "block_withholding") {
            check(s.pool().full_withheld > 0 && s.pool().shares[c.attack.adversary] > s.pool().full_withheld, "Shares and withheld full blocks must differ");
            c.attack.enabled = false;
            sim::Simulator control(c); control.run();
            check(control.pool().full_withheld == 0 && attacks::pool_reward(control) > attacks::pool_reward(s), "Pool loses actual chain rewards against paired honest control");
            check(!control.attack().success(control), "Disabled withholding fails");
        }
        if (name == "sybil") {
            check(s.attack().success(s) && s.attack().filtered_messages > 0, "Victim must actually be isolated by controlled relays");
            for (auto id : c.attack.sybil_identities) check(c.nodes[id].hash_power == 0, "Sybil adds no hash power");
            c.attack.capture_connections = false;
            sim::Simulator connected(c); connected.run();
            check(!connected.attack().success(connected) && connected.nodes()[c.attack.target].chain.known_count() > 1, "An honest path defeats total isolation");
            check(connected.metrics().blocks_produced == s.metrics().blocks_produced, "Identity manipulation adds no discoveries");
            std::vector<sim::Time> first_times, second_times;
            for (const auto& event : s.trace()) if (event.type == "discovery") first_times.push_back(event.time);
            for (const auto& event : connected.trace()) if (event.type == "discovery") second_times.push_back(event.time);
            check(first_times == second_times, "Sybil connectivity must not change the discovery clock");
            auto extra_power = c; extra_power.nodes[c.attack.sybil_identities[0]].hash_power = 1;
            bool refused = false;
            try { sim::Simulator invalid(extra_power); } catch (const std::invalid_argument&) { refused = true; }
            check(refused, "Sybil identities cannot silently gain mining resources");
        }
        sim::Simulator repeat(s.config()); repeat.run();
        check(metrics::results_json(s) == metrics::results_json(repeat), "Attack must reproduce exactly");
        auto invalid = s.config(); invalid.attack.adversary = invalid.nodes.size();
        bool rejected = false;
        try { sim::Simulator bad(invalid); } catch (const std::invalid_argument&) { rejected = true; }
        check(rejected, "Invalid attack participant rejected");
        auto failed_config = sim::load_config(std::string(BSL_SOURCE_DIR) + "/configs/" + name + "_failure.json");
        sim::Simulator expected_failure(failed_config); expected_failure.run();
        check(!expected_failure.attack().success(expected_failure), "Documented failure scenario must fail");
        c.attack.type = "none";
        sim::Simulator honest(c); honest.run();
        check(honest.attack().retained == 0, "Honest control must not withhold");
        std::cout << "PASS " << name << '\n';
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
