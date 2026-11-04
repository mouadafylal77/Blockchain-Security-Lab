#include "attacks/attack.hpp"
#include "sim/config.hpp"
#include <algorithm>
namespace bsl::attacks {
sim::Config network_config(const sim::Config& config) {
    auto c = config;
    if (c.attack.type != "sybil" || !c.attack.enabled) return c;
    if (c.attack.capture_connections) {
        c.links.erase(std::remove_if(c.links.begin(), c.links.end(), [&](const auto& link) {
            return link.from == c.attack.target || link.to == c.attack.target;
        }), c.links.end());
    }
    for (const auto id : c.attack.sybil_identities) {
        const bool exists = std::any_of(c.links.begin(), c.links.end(), [&](const auto& link) {
            return (link.from == id && link.to == c.attack.target) || (link.to == id && link.from == c.attack.target);
        });
        if (!exists) c.links.push_back({id, c.attack.target, 1, 0});
    }
    return c;
}
bool Attack::drop(std::size_t sender, std::size_t recipient) {
    if (config_.type == "sybil" && config_.enabled && recipient == config_.target
        && std::find(config_.sybil_identities.begin(), config_.sybil_identities.end(), sender) != config_.sybil_identities.end()) {
        ++filtered_messages; return true;
    }
    return false;
}
}
