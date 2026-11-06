#include "sim/config.hpp"
#include "core/block.hpp"
#include "detection/config_io.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <set>
#include <cmath>
#include <sstream>
#include <algorithm>

namespace bsl::sim {
using Json = nlohmann::json;
namespace {
void fields(const Json& value, std::initializer_list<const char*> allowed) {
    if (!value.is_object()) throw std::invalid_argument("Expected JSON object");
    const std::set<std::string> names(allowed.begin(), allowed.end());
    for (auto it = value.begin(); it != value.end(); ++it)
        if (!names.count(it.key())) throw std::invalid_argument("Unknown configuration key: " + it.key());
}
std::uint64_t integer(const Json& value) {
    if (!(value.is_number_unsigned() || (value.is_number_integer() && value.get<std::int64_t>() >= 0)))
        throw std::invalid_argument("Expected nonnegative JSON integer");
    return value.get<std::uint64_t>();
}
double number(const Json& value) {
    if (!value.is_number()) throw std::invalid_argument("Expected JSON number");
    return value.get<double>();
}
}
void validate_config(const Config& c) {
    std::set<std::size_t> bootstrap;
    for(const auto peer:c.security.bootstrap_peers)if(peer>=c.nodes.size() || !bootstrap.insert(peer).second)throw std::invalid_argument("Invalid or duplicate public bootstrap candidate");
    if(c.post_target_ticks>1000000000)throw std::invalid_argument("Invalid post_target_ticks");
    attacks::validate(c);
    detection::validate(c.security);
    if((c.security.mode=="baseline" || c.security.mode=="calibration") && c.attack.type!="none" && c.attack.enabled) throw std::invalid_argument("Calibration baseline must be honest");
    if (c.target_height > 10000 || c.observer >= c.nodes.size()) throw std::invalid_argument("Invalid target height/observer");
    if (c.scenario.empty() || c.scenario.size() > 128) throw std::invalid_argument("Invalid scenario name");
    if (!c.duration_ticks || c.duration_ticks > 1000000000000ULL) throw std::invalid_argument("Invalid duration_ticks");
    if (!c.max_events || c.max_events > 1000000 || !c.max_blocks || c.max_blocks > 10000
        || !c.max_pending_events || c.max_pending_events > 1000000) throw std::invalid_argument("Invalid execution limits");
    if (c.difficulty < 1 || c.difficulty > max_difficulty || !c.mining_attempts || c.mining_attempts > 10000000)
        throw std::invalid_argument("Invalid PoW settings");
    if (!std::isfinite(c.mean_block_ticks) || c.mean_block_ticks < 1 || c.mean_block_ticks > 1e9)
        throw std::invalid_argument("Invalid mean_block_ticks");
    if (c.nodes.empty() || c.nodes.size() > 256) throw std::invalid_argument("Expected 1..256 nodes");
    std::set<std::string> ids;
    double total = 0;
    for (const auto& n : c.nodes) {
        if(n.peer_group.size()>64) throw std::invalid_argument("Peer group too long");
        if (n.id.empty() || n.id.size() > 64 || !ids.insert(n.id).second) throw std::invalid_argument("Invalid or duplicate node id");
        if (!std::isfinite(n.hash_power) || n.hash_power < 0 || n.hash_power > 1e12) throw std::invalid_argument("Invalid hash_power");
        total += n.hash_power;
    }
    if (!std::isfinite(total) || total <= 0) throw std::invalid_argument("At least one miner needs positive hash power");
    for (const auto& n : c.nodes) {
        if (n.hash_power > 0 && !std::isfinite(c.mean_block_ticks / (n.hash_power / total)))
            throw std::invalid_argument("Hash power ratio is outside representable mining rates");
    }
    std::set<std::pair<std::size_t, std::size_t>> links;
    for (const auto& l : c.links) {
        if (l.from >= c.nodes.size() || l.to >= c.nodes.size() || l.from == l.to
            || l.latency > 1000000000 || l.jitter > 1000000000) throw std::invalid_argument("Invalid network link");
        if (!links.insert(std::minmax(l.from, l.to)).second) throw std::invalid_argument("Duplicate network link");
    }
}
Config parse_config(const std::string& text) {
    if (text.size() > 1024 * 1024) throw std::invalid_argument("Configuration exceeds 1 MiB");
    // Reject duplicate object keys rather than silently keeping the last value.
    std::vector<std::set<std::string>> keys;
    const auto json = Json::parse(text, [&keys](int depth, Json::parse_event_t event, Json& value) {
        if (depth > 16) throw std::invalid_argument("JSON nesting too deep");
        if (event == Json::parse_event_t::object_start) keys.emplace_back();
        if (event == Json::parse_event_t::key && !keys.back().insert(value.get<std::string>()).second)
            throw std::invalid_argument("Duplicate JSON key");
        if (event == Json::parse_event_t::object_end) keys.pop_back();
        if (keys.size() > 16) throw std::invalid_argument("JSON nesting too deep");
        return true;
    });
    fields(json, {"scenario", "seed", "duration_ticks", "max_events", "max_blocks", "max_pending_events",
        "difficulty", "mining_attempts", "mean_block_ticks", "nodes", "links", "attack", "security", "target_height", "observer", "post_target_ticks"});
    Config c;
    if(json.contains("post_target_ticks"))c.post_target_ticks=integer(json.at("post_target_ticks"));
    if(json.contains("target_height")) c.target_height=integer(json.at("target_height"));
    if(json.contains("observer")) c.observer=integer(json.at("observer"));
    if(json.contains("security")) c.security=detection::parse_config(json.at("security").dump());
    if (json.contains("attack")) c.attack = attacks::parse_config(json.at("attack").dump());
    c.scenario = json.at("scenario").get<std::string>();
    c.seed = integer(json.at("seed"));
    c.duration_ticks = integer(json.at("duration_ticks"));
    c.max_events = integer(json.at("max_events"));
    c.max_blocks = integer(json.at("max_blocks"));
    c.max_pending_events = integer(json.at("max_pending_events"));
    const auto difficulty = integer(json.at("difficulty"));
    if (difficulty > max_difficulty) throw std::invalid_argument("Invalid difficulty");
    c.difficulty = static_cast<unsigned>(difficulty);
    c.mining_attempts = integer(json.at("mining_attempts"));
    c.mean_block_ticks = number(json.at("mean_block_ticks"));
    if (!json.at("nodes").is_array() || !json.at("links").is_array()) throw std::invalid_argument("nodes and links must be arrays");
    std::map<std::string, std::size_t> ids;
    for (const auto& n : json.at("nodes")) {
        fields(n, {"id", "hash_power", "peer_group"});
        const auto id = n.at("id").get<std::string>();
        if (!ids.emplace(id, c.nodes.size()).second) throw std::invalid_argument("Duplicate node id");
        c.nodes.push_back({id, number(n.at("hash_power")), n.value("peer_group",std::string{})});
    }
    for (const auto& l : json.at("links")) {
        fields(l, {"from", "to", "latency_ticks", "jitter_ticks"});
        const auto from = l.at("from").get<std::string>(), to = l.at("to").get<std::string>();
        if (!ids.count(from) || !ids.count(to)) throw std::invalid_argument("Unknown link endpoint");
        c.links.push_back({ids.at(from), ids.at(to), integer(l.at("latency_ticks")), integer(l.at("jitter_ticks"))});
    }
    validate_config(c);
    return c;
}
Config load_config(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) throw std::runtime_error("Cannot open config: " + path);
    std::string text(1024 * 1024 + 1, '\0');
    file.read(text.data(), static_cast<std::streamsize>(text.size()));
    if (file.bad()) throw std::runtime_error("Cannot read config: " + path);
    text.resize(static_cast<std::size_t>(file.gcount()));
    return parse_config(text);
}
std::string config_json(const Config& c) {
    Json nodes = Json::array(), links = Json::array();
    for (const auto& n : c.nodes) nodes.push_back({{"id", n.id}, {"hash_power", n.hash_power}, {"peer_group",n.peer_group}});
    for (const auto& l : c.links) links.push_back({{"from", c.nodes.at(l.from).id}, {"to", c.nodes.at(l.to).id},
        {"latency_ticks", l.latency}, {"jitter_ticks", l.jitter}});
    return Json{{"post_target_ticks",c.post_target_ticks},{"target_height",c.target_height},{"observer",c.observer},{"security",Json::parse(detection::config_json(c.security))},{"attack", Json::parse(attacks::config_json(c.attack))}, {"scenario", c.scenario}, {"seed", c.seed}, {"duration_ticks", c.duration_ticks},
        {"max_events", c.max_events}, {"max_blocks", c.max_blocks}, {"max_pending_events", c.max_pending_events},
        {"difficulty", c.difficulty}, {"mining_attempts", c.mining_attempts}, {"mean_block_ticks", c.mean_block_ticks},
        {"nodes", nodes}, {"links", links}}.dump();
}
}
