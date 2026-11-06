#pragma once
#include "sim/event_queue.hpp"
#include "sim/network.hpp"
#include "sim/node.hpp"
#include "sim/mining_pool.hpp"
#include "detection/feature_extractor.hpp"
#include "mitigation/policy.hpp"
#include "metrics/security_study.hpp"
#include <functional>

namespace bsl::sim {
struct ProducedBlock { Block block; std::size_t miner; Time discovered; };
struct TraceRecord {
    Time time;
    std::size_t node;
    std::string type, hash, old_tip, new_tip;
    std::uint64_t depth = 0;
    std::string detail = {};
    std::uint64_t event_sequence = 0;
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
    void set_observer(std::function<void(const TraceRecord&)> observer) { observer_=std::move(observer); }
    void announce_payment(std::size_t node);
    void record_payment(unsigned observed, unsigned required);
    bool target_reached() const { return height_reached_; }
    Time target_tick() const { return target_tick_; }
    Time production_end_tick() const { return production_end_tick_; }
    bool production_closed() const { return production_closed_; }
    std::uint64_t cancelled_events() const { return cancelled_events_; }
    std::string audit_phase(Time time) const { return audit_phases_.at(time); }
    std::string outcome_reason() const;
    std::string outcome() const;
    const Config& config() const { return config_; }
    const Metrics& metrics() const { return metrics_; }
    const std::vector<Node>& nodes() const { return nodes_; }
    const std::map<std::string, ProducedBlock>& blocks() const { return blocks_; }
    const std::vector<TraceRecord>& trace() const { return trace_; }
    const std::string& stop_reason() const { return stop_reason_; }
    Time now() const { return queue_.now(); }
    std::size_t pending_events() const { return queue_.size(); }
    void publish(std::size_t origin, const std::string& hash) {
        trace_.push_back({now(), origin, "publication", hash, {}, {}, 0});
        receive(origin, hash, true);
    }
    const attacks::Attack& attack() const { return attack_; }
    const MiningPool& pool() const { return pool_; }
    const detection::Detector& detector() const { return detector_; }
    const mitigation::Policy& defenses() const { return defenses_; }
    const Network& network() const { return network_; }
    const std::vector<metrics::MerchantDecision>& merchant_decisions() const { return merchant_decisions_; }
    const std::vector<metrics::DeferredDelivery>& deferred_deliveries() const { return deferred_deliveries_; }
    unsigned required_confirmations(std::size_t node,unsigned base) const { return defenses_.confirmations(node,base,now()); }
    bool payment_allowed(std::size_t node) { ++defenses_.payment_checks; const bool ok=defenses_.payment_allowed(node,now()); if(!ok)++defenses_.payment_deferrals; return ok; }
    std::uint64_t security_audits=0, peak_observation_samples=0;
    std::uint64_t deferred_messages=0, deferred_ticks=0;
    std::uint64_t reconnections=0, sync_messages=0;
private:
    Config config_;
    detection::Detector detector_;
    detection::FeatureExtractor observations_;
    mitigation::Policy defenses_;
    std::vector<Time> last_arrival_;
    std::vector<std::set<std::string>> observed_payloads_;
    std::vector<std::set<std::size_t>> observed_peers_;
    std::vector<std::set<std::size_t>> observed_producers_;
    std::map<std::pair<std::size_t,std::size_t>,std::uint64_t> producer_samples_;
    Time last_audit_=0;
    attacks::Attack attack_;
    MiningPool pool_;
    Rng pool_rng_;
    Rng mining_rng_;
    Network network_;
    EventQueue queue_;
    std::vector<Node> nodes_;
    std::map<std::string, ProducedBlock> blocks_; // Immutable block payloads; no shared chain state.
    std::vector<TraceRecord> trace_;
    std::vector<metrics::MerchantDecision> merchant_decisions_;
    std::vector<metrics::DeferredDelivery> deferred_deliveries_;
    std::vector<bool> reconnection_pending_;
    std::vector<bool> diversification_attempted_;
    Metrics metrics_;
    double total_power_ = 0;
    std::string stop_reason_;
    bool started_ = false;
    bool production_closed_ = false;
    bool height_reached_ = false, payment_status_initialized_=false, payment_active_=false;
    Time target_tick_=0,production_end_tick_=0;
    std::uint64_t cancelled_events_=0;
    std::string production_stop_reason_;
    std::map<Time,std::string> audit_phases_;
    std::vector<bool> notice_seen_;
    std::function<void(const TraceRecord&)> observer_;
    std::uint64_t current_sequence_ = 0;
    void forward_notice(std::size_t node);
    void close_production(const std::string& reason);
    void observe_payment();
    void observe_merchant_decision();
    void schedule_mining(std::size_t miner);
    void schedule_share(std::size_t miner);
    void security_audit(bool operational = true);
    void observe_arrival(const Event& event);
    std::size_t public_producer(const Block& block) const;
    void reconcile_pool();
    void diversify(std::size_t node);
    void complete_diversification(std::size_t node);
    void discover(std::size_t miner);
    void receive(std::size_t node, const std::string& hash, bool local);
    void broadcast(std::size_t node, const std::string& hash);
};
}
