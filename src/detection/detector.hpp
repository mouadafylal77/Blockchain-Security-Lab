#pragma once
#include <cstdint>
#include <cstddef>
#include <map>
#include <string>
#include <vector>
namespace bsl::detection {
using Time = std::uint64_t;
struct Config {
    std::string mode = "off", detector = "fixed", reference_file;
    Time window_ticks = 2000, audit_ticks = 100, cooldown_ticks = 500;
    std::uint64_t min_samples = 5, min_shares = 64;
    double alpha = 0.1, k = 3, clip = 2, drift_cap = 2;
    bool mitigate_confirmations = true, mitigate_publication = true, mitigate_pool = true, mitigate_peers = true;
    Time hold_ticks = 500, publication_delay = 20, confirmation_ticks = 1000;
    Time max_payment_hold_ticks = 0; // 0 preserves unlimited renewal within a risk episode.
    bool event_audits = false, proactive_signals = false;
    bool stratify_reference = false, strict_reference = false;
    bool restrict_bootstrap = false;
    std::vector<std::size_t> bootstrap_peers;
    Time bootstrap_delay_ticks = 0;
    // Opt-in policy revision; legacy scenarios retain their original semantics.
    bool hardened_security = false;
    unsigned preventive_confirmations = 6, persistent_samples = 2;
    double pool_tail_probability = 0.01;
    unsigned confirmation_increment = 3, max_confirmations = 100, max_peers = 4, max_per_group = 1;
    std::map<std::string, double> thresholds{{"reorg_depth", 0.5}, {"fork_rate", 0.005},
        {"late_p95", 40}, {"stale_ratio", 0.25}, {"reward_concentration", 1.1},
        {"share_deficit", 3}, {"peer_concentration", 0.8}, {"silence_ticks", 500}};
};
struct Feature { Time time; std::size_t node; std::string metric; double value; std::string component; bool evidence_sufficient = false; std::uint64_t samples = 0; };
struct Alert {
    Time time; std::size_t node; std::string behavior, metric, component, detector;
    double value, threshold;
    std::string severity = "informative";
    bool evidence_sufficient = false;
    std::uint64_t samples = 0;
};
struct Reference { std::uint64_t count = 0; double mean = 0, variance = 0; };
std::string behavior(const std::string& metric);
void validate(const Config& config);
class Detector {
public:
    explicit Detector(Config config, std::map<std::string, Reference> reference = {});
    std::vector<Alert> observe(const std::vector<Feature>& features);
    const std::vector<Alert>& alerts() const { return alerts_; }
    const std::map<std::string, Reference>& calibration() const { return calibration_; }
    const std::map<std::string, Reference>& reference() const { return reference_; }
    std::uint64_t operations() const { return operations_; }
    std::uint64_t insufficient_samples() const { return insufficient_samples_; }
private:
    Config config_;
    std::map<std::string, Reference> reference_, calibration_, state_;
    std::map<std::string, Time> last_alert_, last_sample_;
    std::map<std::string, unsigned> exceedances_;
    std::vector<Alert> alerts_;
    std::uint64_t operations_ = 0;
    std::uint64_t insufficient_samples_ = 0;
};
}
