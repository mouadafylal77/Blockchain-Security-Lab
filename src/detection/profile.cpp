#include "detection/profile.hpp"
#include "sim/config.hpp"
#include "core/sha256.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
namespace bsl::detection {
std::string network_profile(const sim::Config& c){
    using Json=nlohmann::json;
    const auto normalized=Json::parse(sim::config_json(c));
    return sha256(Json{{"features",c.security.hardened_security?"m4_public_producer_windows_v3":"m4_public_windows_v2"},{"nodes",normalized.at("nodes")},{"links",normalized.at("links")},
        {"difficulty",c.difficulty},{"mean_block_ticks",c.mean_block_ticks},{"window",c.security.window_ticks},{"audit",c.security.audit_ticks},
        {"event_audits",c.security.event_audits},{"min_shares",c.security.min_shares},{"pool_members",c.attack.pool_members},{"shares_model",c.attack.type=="withholding"},
        {"public_notice",c.attack.public_notice},{"merchant_payment_model",c.attack.type=="51"},{"merchant_confirmations",c.attack.confirmations}}.dump());
}
void validate_reference_profile(const sim::Config& c){
    if(c.security.detector!="adaptive" || c.security.mode=="off" || c.security.mode=="calibration")return;
    using Json=nlohmann::json;
    std::ifstream file(c.security.reference_file,std::ios::binary);
    if(!file)throw std::invalid_argument("Cannot read calibration profile");
    std::string text(1024*1024+1,'\0');file.read(text.data(),text.size());text.resize(static_cast<std::size_t>(file.gcount()));
    if(file.bad() || text.size()>1024*1024)throw std::invalid_argument("Invalid reference profile size");
    const auto j=Json::parse(text,[](int depth,Json::parse_event_t,Json&){if(depth>16)throw std::invalid_argument("Reference nesting too deep");return true;});
    if(!j.contains("network_profile")){
        if(c.security.strict_reference || c.security.stratify_reference)throw std::invalid_argument("A bound honest reference is required");
        return; // Explicitly reported legacy compatibility, never claimed as matched.
    }
    if(j.at("network_profile")!=network_profile(c) || j.at("stratified")!=c.security.stratify_reference)
        throw std::invalid_argument("Calibration network/resource/observation profile mismatch");
    const auto& source=j.at("source_config");
    if(source.at("security").at("mode")!="calibration" || (source.at("attack").at("type")!="none" && source.at("attack").at("enabled")!=false))
        throw std::invalid_argument("Reference must declare an honest calibration source");
}
}
