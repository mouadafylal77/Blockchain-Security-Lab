#include "metrics/detection_evaluation.hpp"
#include <algorithm>
namespace bsl::metrics {
Confusion evaluate(const std::vector<detection::Alert>& alerts, const std::string& exposed_family) {
    Confusion c;
    for (const auto* family : {"chain_instability", "publication_anomaly", "pool_deficit", "connectivity_anomaly"}) {
        const bool actual = exposed_family == family;
        const bool predicted = std::any_of(alerts.begin(),alerts.end(),[&](const auto& a){return a.behavior==family;});
        if (actual && predicted) ++c.tp; else if (predicted) ++c.fp; else if(actual) ++c.fn; else ++c.tn;
    }
    return c;
}
}
