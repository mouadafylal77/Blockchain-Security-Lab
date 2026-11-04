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
    Time hold_ticks = 500, publication_delay = 20;
    unsigned confirmation_increment = 3, max_confirmations = 100, max_peers = 4, max_per_group = 1;
    std::map<std::string, double> thresholds{{"reorg_depth", 0.5}, {"fork_rate", 0.005},
        {"late_p95", 40}, {"stale_ratio", 0.25}, {"reward_concentration", 1.1},
        {"share_deficit", 3}, {"peer_concentration", 0.8}, {"silence_ticks", 500}};
};
struct Feature { Time time; std::size_t node; std::string metric; double value; std::string component; };
struct Alert {
    Time time; std::size_t node; std::string behavior, metric, component, detector;
    double value, threshold;
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
    std::vector<Alert> alerts_;
    std::uint64_t operations_ = 0;
    std::uint64_t insufficient_samples_ = 0;
};
}
