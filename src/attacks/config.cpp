#include "attacks/attack.hpp"
#include "sim/config.hpp"
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <algorithm>
namespace bsl::attacks {
Config parse_config(const std::string& text) {
    const auto j = nlohmann::json::parse(text);
    if (!j.is_object()) throw std::invalid_argument("attack must be an object");
    for (auto it = j.begin(); it != j.end(); ++it)
        if (it.key() != "type" && it.key() != "adversary" && it.key() != "target" && it.key() != "confirmations" && it.key() != "enabled" && it.key() != "pool_members" && it.key() != "sybil_identities" && it.key() != "capture_connections" && it.key() != "adversaries" && it.key() != "public_notice")
            throw std::invalid_argument("Unknown attack field");
    Config c;
    if(j.contains("public_notice")) c.public_notice=j.at("public_notice").get<bool>();
    if(j.contains("adversaries")){
        if(!j.at("adversaries").is_array())throw std::invalid_argument("adversaries must be an array");
        for(const auto& v:j.at("adversaries")){if(!v.is_number_unsigned())throw std::invalid_argument("Unsigned coalition index required");c.adversaries.push_back(v.get<std::size_t>());}
    }
    if (j.contains("enabled")) c.enabled = j.at("enabled").get<bool>();
    if (j.contains("capture_connections")) c.capture_connections = j.at("capture_connections").get<bool>();
    if (j.contains("sybil_identities")) {
        if (!j.at("sybil_identities").is_array()) throw std::invalid_argument("sybil_identities must be an array");
        for (const auto& value : j.at("sybil_identities")) {
            if (!value.is_number_unsigned()) throw std::invalid_argument("Sybil requires unsigned index");
            c.sybil_identities.push_back(value.get<std::size_t>());
        }
    }
    if (j.contains("pool_members")) {
        if (!j.at("pool_members").is_array()) throw std::invalid_argument("pool_members must be an array");
        for (const auto& value : j.at("pool_members")) {
            if (!value.is_number_unsigned()) throw std::invalid_argument("Pool member requires unsigned index");
            c.pool_members.push_back(value.get<std::size_t>());
        }
    }
    c.type = j.at("type").get<std::string>();
    for (const auto* name : {"adversary", "target", "confirmations"})
        if (!j.at(name).is_number_unsigned()) throw std::invalid_argument("Attack indices/confirmations require unsigned integers");
    c.adversary = j.at("adversary").get<std::size_t>();
    c.target = j.at("target").get<std::size_t>();
    const auto confirmations = j.at("confirmations").get<std::uint64_t>();
    if (confirmations > 10000) throw std::invalid_argument("Too many confirmations");
    c.confirmations = static_cast<unsigned>(confirmations);
    return c;
}
std::string config_json(const Config& c) {
    return nlohmann::json{{"adversaries",c.adversaries},{"public_notice",c.public_notice},{"type",c.type},{"adversary",c.adversary},{"target",c.target},{"confirmations",c.confirmations},{"enabled",c.enabled},{"pool_members",c.pool_members},{"sybil_identities",c.sybil_identities},{"capture_connections",c.capture_connections}}.dump();
}
void validate(const sim::Config& c) {
    if (c.attack.type == "none") return;
    if (c.attack.type != "51" && c.attack.type != "selfish" && c.attack.type != "withholding" && c.attack.type != "sybil") throw std::invalid_argument("Unknown attack type");
    if (c.attack.adversary >= c.nodes.size() || c.attack.target >= c.nodes.size()
        || c.attack.adversary == c.attack.target || !c.attack.confirmations || c.attack.confirmations > 10000)
        throw std::invalid_argument("Invalid attack participants or confirmations");
    std::vector<std::size_t> coalition;
    for(auto id:c.attack.adversaries){
        if(id>=c.nodes.size() || id==c.attack.target || std::find(coalition.begin(),coalition.end(),id)!=coalition.end())throw std::invalid_argument("Invalid adversarial coalition");
        coalition.push_back(id);
    }
    if(!coalition.empty() && std::find(coalition.begin(),coalition.end(),c.attack.adversary)==coalition.end())throw std::invalid_argument("Coalition must include its controller");
    if (c.attack.type == "withholding") {
        if (c.difficulty < 2 || c.attack.pool_members.empty()) throw std::invalid_argument("Pool requires difficulty >= 2 and members");
        std::vector<std::size_t> seen;
        for (const auto member : c.attack.pool_members) {
            if (member >= c.nodes.size() || std::find(seen.begin(), seen.end(), member) != seen.end()) throw std::invalid_argument("Invalid pool member");
            seen.push_back(member);
        }
        for(auto id:coalition)if(std::find(seen.begin(),seen.end(),id)==seen.end())throw std::invalid_argument("All withholding miners must belong to pool");
        if (std::find(seen.begin(), seen.end(), c.attack.adversary) == seen.end()) throw std::invalid_argument("Adversary must belong to pool");
    }
    if (c.attack.type == "sybil") {
        if (c.attack.sybil_identities.empty() || c.nodes[c.attack.target].hash_power != 0) throw std::invalid_argument("Sybil scenario requires identities and a passive target");
        std::vector<std::size_t> seen;
        for (const auto id : c.attack.sybil_identities) {
            if (id >= c.nodes.size() || id == c.attack.target || id == c.attack.adversary || c.nodes[id].hash_power != 0
                || std::find(seen.begin(), seen.end(), id) != seen.end()) throw std::invalid_argument("Sybil identities must be unique zero-power nodes");
            seen.push_back(id);
        }
    }
}
}
