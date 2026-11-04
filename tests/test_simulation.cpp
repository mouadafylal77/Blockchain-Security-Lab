#include "core/block_tree.hpp"
#include "core/blockchain.hpp"
#include "sim/simulator.hpp"
#include "metrics/json_exporter.hpp"
#include <nlohmann/json.hpp>
#include <functional>
#include <iostream>
#include <limits>

using namespace bsl;
using namespace bsl::sim;
using Json = nlohmann::json;
namespace {
void require(bool value, const std::string& message) { if (!value) throw std::runtime_error(message); }
template<class Function> void rejects(Function f) {
    bool threw = false;
    try { f(); } catch (const std::exception&) { threw = true; }
    require(threw, "Expected rejection");
}
Block child(const Block& parent, const std::string& label, unsigned difficulty = 1) {
    Block b;
    b.index = parent.index + 1;
    b.previous_hash = parent.hash;
    b.simulated_time = parent.simulated_time + 1;
    b.transactions = {label};
    b.difficulty = difficulty;
    return mine_block(b);
}
Config small_config() {
    Config c;
    c.nodes = {{"miner", 100}, {"relay", 0}, {"observer", 0}};
    c.links = {{0, 1, 7, 0}, {1, 2, 7, 0}};
    c.mean_block_ticks = 30;
    c.duration_ticks = 10000;
    c.max_blocks = 3;
    return c;
}
void event_order() {
    EventQueue q(10);
    q.push(Event{5, 999, EventType::mine, 0, 0, {}});
    q.push(Event{2, 999, EventType::deliver, 1, 0, "a"});
    q.push(Event{2, 0, EventType::mine, 2, 0, {}});
    require(q.pop().node == 1 && q.now() == 2, "Chronological order");
    q.push(Event{2, 0, EventType::mine, 3, 0, {}});
    require(q.pop().node == 2, "Existing equal timestamp precedes new event");
    require(q.pop().node == 3, "Insertion sequence breaks ties");
    require(q.pop().time == 5 && q.empty(), "Final clock");
    rejects([&] { q.pop(); });
    rejects([&] { q.push(Event{4, 0, EventType::mine, 0, 0, {}}); });
    EventQueue bounded(1);
    bounded.push(Event{});
    rejects([&] { bounded.push(Event{}); });
}
void rng() {
    Rng a(42), b(42), other(43);
    bool changed = false;
    for (int i = 0; i < 100; ++i) {
        const auto x = a.exponential_ticks(100);
        require(x == b.exponential_ticks(100) && x > 0, "Seeded positive wait");
        changed |= x != other.exponential_ticks(100);
        const auto jitter = a.uniform_ticks(7);
        require(jitter == b.uniform_ticks(7) && jitter <= 7, "Seeded bounded jitter");
        require(a.uniform_ticks(0) == 0 && b.uniform_ticks(0) == 0, "Zero jitter");
    }
    require(changed, "Different seeds vary waiting times");
    rejects([&] { a.exponential_ticks(0); });
    rejects([&] { a.exponential_ticks(std::numeric_limits<double>::quiet_NaN()); });
}
void network() {
    Simulator sim(small_config());
    sim.run();
    require(sim.metrics().blocks_produced == 3, "Three discoveries");
    for (const auto& n : sim.nodes()) {
        require(n.chain.known_count() == 4 && n.chain.pending_count() == 0, "Delivery over two hops");
        require(n.chain.tip().hash == sim.nodes()[0].chain.tip().hash, "Convergence");
    }
    require(sim.metrics().propagation_samples == 6 && sim.metrics().propagation_sum_ticks == 63,
        "First-arrival delays: 7 + 14 ticks per block");
    require(sim.metrics().messages_sent == sim.metrics().messages_delivered, "Network drained");
    for (const auto& p : sim.blocks()) require(p.second.miner == 0, "Zero-power nodes never mine");
}
void local_views() {
    auto c = small_config();
    c.links.clear();
    Simulator isolated(c);
    isolated.run();
    require(isolated.nodes()[0].chain.known_count() == 4, "Miner sees its blocks");
    require(isolated.nodes()[1].chain.known_count() == 1, "Disconnected node sees genesis only");
    BlockTree a, b;
    const auto block = child(genesis_block(), "local");
    a.receive(block);
    require(a.tip().hash != b.tip().hash, "Views diverge before delivery");
    b.receive(block);
    require(a.tip().hash == b.tip().hash, "Views converge after delivery");
}
void network_disorder() {
    auto c = small_config();
    c.nodes = {{"miner", 1}, {"observer", 0}};
    c.links = {{0, 1, 0, 50}};
    c.mean_block_ticks = 1;
    c.max_blocks = 20;
    Simulator shuffled(c); shuffled.run();
    require(shuffled.metrics().unknown_parent_arrivals > 0, "Jitter really delivers children before parents");
    require(shuffled.nodes()[1].chain.pending_count() == 0 && shuffled.nodes()[1].chain.known_count() == 21,
        "Network disorder resolves when parents arrive");
    require(shuffled.nodes()[0].chain.tip().hash == shuffled.nodes()[1].chain.tip().hash, "Convergence despite disorder");
    c.links = {{0, 1, 0, 0}};
    Simulator instant(c); instant.run();
    require(instant.metrics().propagation_max_ticks == 0 && instant.pending_events() == 0,
        "Zero-latency gossip terminates without real waits");
}
void hash_power() {
    auto c = small_config();
    c.nodes = {{"large", 90}, {"small", 10}, {"observer1", 0}, {"observer2", 0}};
    c.links.clear();
    c.duration_ticks = 100000;
    c.max_blocks = 200;
    Simulator s(c); s.run();
    std::uint64_t large = 0, small = 0;
    for (const auto& item : s.blocks()) {
        require(item.second.miner < 2, "Extra network identities receive no hash power");
        if (item.second.miner == 0) ++large; else ++small;
    }
    require(large > small && small > 0 && large + small == 200, "Configured hash power affects discovery distribution");
}
void pending() {
    BlockTree tree;
    const auto a = child(genesis_block(), "parent");
    const auto b = child(a, "child");
    const auto c = child(b, "grandchild");
    require(tree.receive(c).status == ReceiveStatus::pending_parent, "Grandchild buffered");
    require(tree.receive(b).status == ReceiveStatus::pending_parent, "Child buffered");
    require(tree.known_count() == 1 && tree.pending_count() == 2 && tree.stale_count() == 0,
        "Unknown parents are not connected stale blocks");
    const auto result = tree.receive(a);
    require(result.updates.size() == 3 && tree.pending_count() == 0 && tree.tip().hash == c.hash,
        "Recursive attachment after parent arrival");
    BlockTree invalid_pending;
    auto bad = b;
    bad.index = 99;
    bad = mine_block(bad);
    invalid_pending.receive(bad);
    invalid_pending.receive(a);
    require(invalid_pending.pending_count() == 0 && !invalid_pending.contains(bad.hash), "Invalid waiting child discarded");
}
void duplicates() {
    BlockTree tree;
    const auto a = child(genesis_block(), "a"), b = child(a, "b");
    tree.receive(b);
    require(tree.receive(b).status == ReceiveStatus::duplicate && tree.pending_count() == 1, "Pending duplicate suppressed");
    tree.receive(a);
    const auto result = tree.receive(a);
    require(result.status == ReceiveStatus::duplicate && result.updates.empty() && tree.known_count() == 3, "Connected duplicate suppressed");
    require(tree.receive(genesis_block()).status == ReceiveStatus::duplicate, "Genesis duplicate");
}
void fork_choice() {
    const auto g = genesis_block();
    const auto a = child(g, "light"), a2 = child(a, "light2"), heavy = child(g, "heavy", 2);
    BlockTree tree;
    tree.receive(a);
    tree.receive(a2);
    const auto change = tree.receive(heavy);
    require(tree.tip().hash == heavy.hash && tree.cumulative_work(heavy.hash) == 256, "Work beats height");
    require(tree.cumulative_work(a2.hash) == 32 && change.updates[0].fork, "Sibling fork recognized");
    const auto b = child(g, "tie");
    BlockTree first, second;
    first.receive(a); first.receive(b);
    second.receive(b); second.receive(a);
    require(first.tip().hash == std::min(a.hash, b.hash) && first.tip().hash == second.tip().hash,
        "Lexicographic tie break independent of arrival order");
}
void reorganization() {
    const auto g = genesis_block(), a = child(g, "a"), b = child(a, "b"), c = child(b, "c");
    BlockTree tree;
    for (const auto& block : {a, b, c}) {
        require(tree.receive(block).updates[0].detached_depth == 0, "Extension is not a reorganization");
    }
    const auto heavy = child(g, "heavy-reorg", 2);
    const auto update = tree.receive(heavy).updates[0];
    require(update.detached_depth == 3 && update.old_tip == c.hash && update.new_tip == heavy.hash, "Reorg depth from common ancestor");
    require(tree.stale_count() == 3 && tree.active_chain() == std::vector<std::string>{g.hash, heavy.hash}, "Detached blocks remain valid stale blocks");
}
void invalid_blocks() {
    BlockTree tree;
    const auto a = child(genesis_block(), "a");
    auto bad = a;
    bad.transactions = {"tampered"};
    require(tree.receive(bad).status == ReceiveStatus::invalid, "Tampered payload");
    tree.receive(a);
    auto b = child(a, "b");
    b.simulated_time = 0;
    b = mine_block(b);
    require(tree.receive(b).status == ReceiveStatus::invalid, "Backwards timestamp");
    auto foreign = genesis_block();
    foreign.transactions = {"foreign"};
    require(tree.receive(foreign).status == ReceiveStatus::invalid, "Foreign genesis");
    bad = a;
    bad.difficulty = 0;
    require(tree.receive(bad).status == ReceiveStatus::invalid, "Invalid work even for a known hash");
}
void reproducibility() {
    const auto config = load_config(std::string(BSL_SOURCE_DIR) + "/configs/fork_reorg.json");
    Simulator a(config), b(config);
    a.run(); b.run();
    require(metrics::results_json(a) == metrics::results_json(b), "Complete JSON byte-identical for same configuration and seed");
    auto changed = config;
    ++changed.seed;
    Simulator other(changed);
    other.run();
    require(a.blocks().begin()->first != other.blocks().begin()->first, "Different seed changes discoveries");
    rejects([&] { a.run(); });
}
void config_invalid() {
    const auto good = config_json(small_config());
    require(config_json(parse_config(good)) == good, "JSON round trip");
    const std::vector<std::function<void(Json&)>> invalid = {
        [](auto& j) { j["seed"] = -1; }, [](auto& j) { j["seed"] = 1.5; },
        [](auto& j) { j["duration_ticks"] = 0; }, [](auto& j) { j["duration_ticks"] = true; },
        [](auto& j) { j["max_events"] = 0; }, [](auto& j) { j["max_blocks"] = 10001; },
        [](auto& j) { j["max_pending_events"] = 0; }, [](auto& j) { j["difficulty"] = 4294967297ULL; },
        [](auto& j) { j["difficulty"] = 0; }, [](auto& j) { j["mining_attempts"] = 0; },
        [](auto& j) { j["mean_block_ticks"] = "100"; }, [](auto& j) { j["mean_block_ticks"] = 0; },
        [](auto& j) { j["nodes"][0]["hash_power"] = -1; },
        [](auto& j) { j["nodes"][0]["hash_power"] = 0; },
        [](auto& j) { j["nodes"][1]["id"] = "miner"; },
        [](auto& j) { j["nodes"] = Json::array(); },
        [](auto& j) { j["links"][0]["to"] = "missing"; },
        [](auto& j) { j["links"][0]["to"] = "miner"; },
        [](auto& j) { j["links"][0]["latency_ticks"] = -1; },
        [](auto& j) { j["links"].push_back(j["links"][0]); },
        [](auto& j) { j["typo"] = 1; }, [](auto& j) { j.erase("nodes"); }
    };
    for (const auto& mutate : invalid) { auto j = Json::parse(good); mutate(j); rejects([&] { parse_config(j.dump()); }); }
    for (const std::string text : {"{", "{} trailing", "{\"seed\":1,\"seed\":2}", "[]", "null"})
        rejects([&] { parse_config(text); });
    auto wide_seed = small_config();
    wide_seed.seed = std::numeric_limits<std::uint64_t>::max();
    require(parse_config(config_json(wide_seed)).seed == wide_seed.seed, "Seed retains all 64 bits");
    auto c = small_config();
    c.mean_block_ticks = std::numeric_limits<double>::infinity();
    rejects([&] { Simulator s(c); });
    c = small_config(); c.nodes[1].hash_power = std::numeric_limits<double>::denorm_min();
    rejects([&] { validate_config(c); });
    rejects([&] { load_config("no-such-config-file.json"); });
}
void limits() {
    auto c = small_config();
    c.max_events = 1;
    Simulator bounded(c); bounded.run();
    require(bounded.metrics().events == 1 && bounded.stop_reason() == "max_events", "Event budget stops execution");
    c = small_config(); c.duration_ticks = 1; c.mean_block_ticks = 1000000;
    Simulator horizon(c); horizon.run();
    require(horizon.metrics().blocks_produced == 0 && horizon.stop_reason() == "duration_ticks", "Horizon without discoveries");
    c = small_config(); c.max_blocks = 1;
    Simulator blocks(c); blocks.run();
    require(blocks.metrics().blocks_produced == 1 && blocks.stop_reason() == "max_blocks_drained" && blocks.pending_events() == 0,
        "Mining stops, deliveries drain");
    c = small_config(); c.max_pending_events = 1;
    rejects([&] { Simulator s(c); s.run(); });
    BlockTree capacity(1);
    rejects([&] { capacity.receive(child(genesis_block(), "overflow")); });
    c = small_config(); c.difficulty = 6; c.mining_attempts = 1;
    rejects([&] { Simulator s(c); s.run(); });
}
void metrics_test() {
    Simulator s(small_config()); s.run();
    const auto j = Json::parse(metrics::results_json(s));
    require(j.at("schema_version") == 1 && j.at("config").at("seed") == 42, "Structured metadata");
    require(j.at("nodes")[0].at("blocks_mined") == 3 && j.at("nodes")[1].at("blocks_mined") == 0, "Origin distribution");
    require(j.at("metrics").at("propagation_mean_ticks") == 10.5, "Propagation mean definition");
    require(s.metrics().forks == 0 && s.metrics().reorganizations == 0, "Single miner baseline has no forks");
    for (const auto& n : s.nodes()) {
        std::vector<Block> active{genesis_block()};
        for (const auto& hash : n.chain.active_chain()) if (hash != genesis_block().hash) active.push_back(s.blocks().at(hash).block);
        require(is_valid_chain(active), "Active chains remain valid under M1 validation");
    }
}
void demo_fork() {
    Simulator s(load_config(std::string(BSL_SOURCE_DIR) + "/configs/fork_reorg.json")); s.run();
    require(s.metrics().forks > 0 && s.metrics().reorganizations > 0, "Demo really creates forks and reorgs");
    require(s.metrics().blocks_produced == 12 && s.pending_events() == 0, "Twelve blocks then drain");
    for (const auto& n : s.nodes()) {
        require(n.chain.tip().hash == s.nodes()[0].chain.tip().hash, "Demo converges");
        require(n.chain.stale_count() > 0 && n.chain.pending_count() == 0, "Stale and unknown-parent distinction");
    }
}
}
int main(int argc, char* argv[]) {
    const std::map<std::string, std::function<void()>> tests = {
        {"event_order", event_order}, {"rng", rng}, {"network", network}, {"network_disorder", network_disorder},
        {"hash_power", hash_power}, {"local_views", local_views},
        {"pending", pending}, {"duplicates", duplicates}, {"fork_choice", fork_choice},
        {"reorganization", reorganization}, {"invalid_blocks", invalid_blocks},
        {"reproducibility", reproducibility}, {"config_invalid", config_invalid}, {"limits", limits},
        {"metrics", metrics_test}, {"demo_fork", demo_fork}
    };
    try {
        if (argc != 2 || tests.count(argv[1]) == 0) throw std::invalid_argument("Unknown test");
        tests.at(argv[1])();
        std::cout << "PASS " << argv[1] << '\n'; return 0;
    } catch (const std::exception& e) { std::cerr << "FAIL " << e.what() << '\n'; return 1; }
}
