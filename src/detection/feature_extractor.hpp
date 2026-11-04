#pragma once
#include "detection/detector.hpp"
#include <deque>
#include <set>
namespace bsl::detection {
// Public telemetry only: no simulator, attack labels, private blocks or hash-power input.
class FeatureExtractor {
public:
    explicit FeatureExtractor(Time window) : window_(window) {}
    void record(Time time, std::size_t node, const std::string& metric, double value, const std::string& component = "");
    std::vector<double> values(Time now, std::size_t node, const std::string& metric, const std::string& component = "");
    std::size_t retained_samples() const;
private:
    struct Sample { Time time; double value; };
    Time window_;
    std::map<std::string, std::deque<Sample>> samples_;
};
}
