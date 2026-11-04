#pragma once
#include "sim/event_queue.hpp"
#include "sim/network.hpp"
#include "sim/node.hpp"

namespace bsl::sim {
struct ProducedBlock { Block block; std::size_t miner; Time discovered; };
struct TraceRecord {
    Time time;
    std::size_t node;
    std::string type, hash, old_tip, new_tip;
    std::uint64_t depth = 0;
};
struct Metrics {
    std::uint64_t events = 0, blocks_produced = 0, messages_sent = 0, messages_delivered = 0;
    std::uint64_t duplicates = 0, unknown_parent_arrivals = 0, forks = 0, reorganizations = 0;
    std::uint64_t propagation_samples = 0, propagation_sum_ticks = 0, propagation_max_ticks = 0;
};
class Simulator {
public:
    explicit Simulator(Config config);
    void run(); // Exactly once per instance.
    const Config& config() const { return config_; }
    const Metrics& metrics() const { return metrics_; }
    const std::vector<Node>& nodes() const { return nodes_; }
    const std::map<std::string, ProducedBlock>& blocks() const { return blocks_; }
    const std::vector<TraceRecord>& trace() const { return trace_; }
    const std::string& stop_reason() const { return stop_reason_; }
    Time now() const { return queue_.now(); }
    std::size_t pending_events() const { return queue_.size(); }
private:
    Config config_;
    Rng mining_rng_;
    Network network_;
    EventQueue queue_;
    std::vector<Node> nodes_;
    std::map<std::string, ProducedBlock> blocks_; // Immutable block payloads; no shared chain state.
    std::vector<TraceRecord> trace_;
    Metrics metrics_;
    double total_power_ = 0;
    std::string stop_reason_;
    bool started_ = false;
    void schedule_mining(std::size_t miner);
    void discover(std::size_t miner);
    void receive(std::size_t node, const std::string& hash, bool local);
    void broadcast(std::size_t node, const std::string& hash);
};
}
