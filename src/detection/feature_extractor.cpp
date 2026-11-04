#include "detection/feature_extractor.hpp"
#include <stdexcept>
#include <cmath>
namespace bsl::detection {
namespace { std::string key(std::size_t node, const std::string& metric, const std::string& component) { return std::to_string(node)+"/"+metric+"/"+component; } }
void FeatureExtractor::record(Time time, std::size_t node, const std::string& metric, double value, const std::string& component) {
    if (!window_ || !std::isfinite(value) || value < 0) throw std::invalid_argument("Invalid telemetry");
    auto& q = samples_[key(node,metric,component)];
    if (!q.empty() && time < q.back().time) throw std::invalid_argument("Telemetry cannot go backwards");
    q.push_back({time,value});
}
std::vector<double> FeatureExtractor::values(Time now, std::size_t node, const std::string& metric, const std::string& component) {
    auto& q = samples_[key(node,metric,component)];
    if (!q.empty() && q.back().time > now) throw std::invalid_argument("Future telemetry is inaccessible");
    while (!q.empty() && now >= window_ && q.front().time <= now - window_) q.pop_front();
    std::vector<double> result;
    for (const auto& s : q) result.push_back(s.value);
    return result;
}
std::size_t FeatureExtractor::retained_samples() const { std::size_t n=0; for (const auto& item:samples_) n+=item.second.size(); return n; }
}
