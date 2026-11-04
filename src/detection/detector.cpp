#include "detection/detector.hpp"
#include <cmath>
#include <algorithm>
#include <stdexcept>
namespace bsl::detection {
std::string behavior(const std::string& metric) {
    if (metric == "reorg_depth") return "chain_instability";
    if (metric == "share_deficit") return "pool_deficit";
    if (metric == "peer_concentration" || metric == "silence_ticks") return "connectivity_anomaly";
    return "publication_anomaly";
}
void validate(const Config& c) {
    if (c.mode != "off" && c.mode != "calibration" && c.mode != "baseline" && c.mode != "attacked" && c.mode != "protected") throw std::invalid_argument("Invalid security mode");
    if (c.detector != "fixed" && c.detector != "adaptive") throw std::invalid_argument("Invalid detector");
    if (!c.audit_ticks || c.audit_ticks > c.window_ticks || c.window_ticks > 1000000000 || !c.cooldown_ticks || c.cooldown_ticks > 1000000000
        || c.min_samples < 2 || c.min_samples > 100000 || !c.min_shares || c.min_shares > 1000000)
        throw std::invalid_argument("Invalid observation window or sample limits");
    for (const auto x : {c.alpha, c.k, c.clip, c.drift_cap}) if (!std::isfinite(x) || x <= 0) throw std::invalid_argument("Invalid adaptive parameter");
    if (c.alpha > 1 || c.k > 20 || c.clip > 10 || c.drift_cap > 10) throw std::invalid_argument("Adaptive parameter out of range");
    if (!c.hold_ticks || c.hold_ticks > 1000000000 || !c.publication_delay || c.publication_delay > 1000000000
        || !c.confirmation_increment || c.max_confirmations < c.confirmation_increment || c.max_confirmations > 10000
        || !c.max_peers || c.max_peers > 256 || !c.max_per_group || c.max_per_group > c.max_peers)
        throw std::invalid_argument("Invalid mitigation parameter");
    const Config defaults;
    if (c.thresholds.size() != defaults.thresholds.size()) throw std::invalid_argument("Incomplete metric thresholds");
    for (const auto& item : c.thresholds) if (!defaults.thresholds.count(item.first) || !std::isfinite(item.second) || item.second <= 0 || item.second > 1e15)
        throw std::invalid_argument("Invalid metric threshold");
}
Detector::Detector(Config config, std::map<std::string, Reference> reference) : config_(std::move(config)), reference_(std::move(reference)) {
    validate(config_);
    for (const auto& item : reference_) if (!config_.thresholds.count(item.first) || item.second.count < config_.min_samples
        || !std::isfinite(item.second.mean) || item.second.mean < 0 || item.second.mean > 1e15
        || !std::isfinite(item.second.variance) || item.second.variance < 0 || item.second.variance > 1e30)
        throw std::invalid_argument("Invalid or insufficient calibration");
}
std::vector<Alert> Detector::observe(const std::vector<Feature>& features) {
    std::vector<Alert> generated;
    for (const auto& f : features) {
        if (!config_.thresholds.count(f.metric) || !std::isfinite(f.value) || f.value < 0 || f.value > 1e15) throw std::invalid_argument("Invalid observable feature");
        const auto key = std::to_string(f.node) + "/" + f.component + "/" + f.metric;
        if (last_sample_.count(key) && f.time <= last_sample_.at(key)) throw std::invalid_argument("Feature time must increase");
        last_sample_[key] = f.time;
        ++operations_;
        if (config_.mode == "calibration") {
            auto& r = calibration_[f.metric];
            const auto previous_count = r.count++;
            const double delta = f.value - r.mean;
            r.mean += delta / static_cast<double>(r.count);
            r.variance = (r.variance * previous_count + delta * (f.value - r.mean)) / r.count;
        }
        if (config_.mode == "off" || config_.mode == "calibration") continue;
        double threshold = config_.thresholds.at(f.metric);
        Reference* state = nullptr;
        double sigma = 0;
        const double floor = config_.thresholds.at(f.metric) * 0.1;
        if (config_.detector == "adaptive") {
            if (!reference_.count(f.metric)) { ++insufficient_samples_; continue; }
            if (!state_.count(key)) state_[key] = reference_.at(f.metric);
            state = &state_.at(key);
            sigma = std::sqrt(std::max(state->variance, floor * floor));
            threshold = std::max(threshold, state->mean + config_.k * sigma);
        }
        if (f.value > threshold && (!last_alert_.count(key) || f.time - last_alert_[key] >= config_.cooldown_ticks)) {
            Alert a{f.time, f.node, behavior(f.metric), f.metric, f.component, config_.detector, f.value, threshold};
            generated.push_back(a); alerts_.push_back(a); last_alert_[key] = f.time;
        }
        // Score first. Exceedances never train the detector, even during cooldown.
        if (state && f.value <= threshold) {
            const auto& anchor = reference_.at(f.metric);
            const double anchor_sigma = std::sqrt(std::max(anchor.variance, floor * floor));
            const double x = std::clamp(f.value, state->mean - config_.clip * sigma, state->mean + config_.clip * sigma);
            const double delta = x - state->mean;
            state->mean = std::clamp(state->mean + config_.alpha * delta, 0.0, anchor.mean + config_.drift_cap * anchor_sigma);
            state->variance = std::min((1-config_.alpha) * (state->variance + config_.alpha * delta * delta),
                4 * anchor_sigma * anchor_sigma);
        }
    }
    return generated;
}
}
