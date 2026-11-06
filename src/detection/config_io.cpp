#include "detection/config_io.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <stdexcept>
namespace bsl::detection {
using Json=nlohmann::json;
std::string config_json(const Config& c) {
    return Json{{"mode",c.mode},{"detector",c.detector},{"reference_file",c.reference_file},
        {"window_ticks",c.window_ticks},{"audit_ticks",c.audit_ticks},{"cooldown_ticks",c.cooldown_ticks},
        {"min_samples",c.min_samples},{"min_shares",c.min_shares},{"alpha",c.alpha},{"k",c.k},{"clip",c.clip},{"drift_cap",c.drift_cap},
        {"thresholds",c.thresholds},{"hold_ticks",c.hold_ticks},{"confirmation_ticks",c.confirmation_ticks},{"event_audits",c.event_audits},{"proactive_signals",c.proactive_signals},{"publication_delay",c.publication_delay},
        {"stratify_reference",c.stratify_reference},{"strict_reference",c.strict_reference},
        {"restrict_bootstrap",c.restrict_bootstrap},{"bootstrap_peers",c.bootstrap_peers},{"bootstrap_delay_ticks",c.bootstrap_delay_ticks},
        {"max_payment_hold_ticks",c.max_payment_hold_ticks},{"confirmation_increment",c.confirmation_increment},{"max_confirmations",c.max_confirmations},
        {"hardened_security",c.hardened_security},{"preventive_confirmations",c.preventive_confirmations},
        {"persistent_samples",c.persistent_samples},{"pool_tail_probability",c.pool_tail_probability},
        {"max_peers",c.max_peers},{"max_per_group",c.max_per_group},
        {"mitigate_confirmations",c.mitigate_confirmations},{"mitigate_publication",c.mitigate_publication},
        {"mitigate_pool",c.mitigate_pool},{"mitigate_peers",c.mitigate_peers}}.dump();
}
Config parse_config(const std::string& text) {
    const auto j=Json::parse(text);const auto defaults=Json::parse(config_json(Config{}));
    if(!j.is_object())throw std::invalid_argument("security must be an object");
    auto merged=defaults;
    for(auto it=j.begin();it!=j.end();++it){
        if(!defaults.contains(it.key()))throw std::invalid_argument("Unknown security parameter: "+it.key());
        if(it.key()=="thresholds"){
            if(!it->is_object())throw std::invalid_argument("thresholds must be an object");
            for(auto q=it->begin();q!=it->end();++q){if(!defaults["thresholds"].contains(q.key())||!q->is_number())throw std::invalid_argument("Invalid threshold");merged["thresholds"][q.key()]=q.value();}
        }else if(it.key()=="bootstrap_peers"){
            if(!it->is_array())throw std::invalid_argument("bootstrap_peers must be an array");
            for(const auto& value:it.value())if(!value.is_number_unsigned())throw std::invalid_argument("Unsigned bootstrap index required");
            merged[it.key()]=it.value();
        }else{
            const auto& d=defaults.at(it.key());
            if(d.is_number_unsigned() && !it->is_number_unsigned())throw std::invalid_argument("Security count must be unsigned integer");
            if(d.is_boolean() && !it->is_boolean())throw std::invalid_argument("Security switch must be boolean");
            if(d.is_string() && !it->is_string())throw std::invalid_argument("Security name must be string");
            if(d.is_number_float() && !it->is_number())throw std::invalid_argument("Security parameter must be numeric");
            if(d.is_number_unsigned() && it->get<std::uint64_t>()>1000000000ULL)throw std::invalid_argument("Security integer out of range");
            merged[it.key()]=it.value();
        }
    }
    Config c;
#define READ(field) c.field=merged.at(#field).get<decltype(c.field)>()
    READ(mode);READ(detector);READ(reference_file);READ(window_ticks);READ(audit_ticks);READ(cooldown_ticks);READ(stratify_reference);READ(strict_reference);
    READ(restrict_bootstrap);READ(bootstrap_peers);READ(bootstrap_delay_ticks);
    READ(hardened_security);READ(preventive_confirmations);READ(persistent_samples);READ(pool_tail_probability);
    READ(min_samples);READ(min_shares);READ(alpha);READ(k);READ(clip);READ(drift_cap);READ(thresholds);
    READ(hold_ticks);READ(confirmation_ticks);READ(max_payment_hold_ticks);READ(event_audits);READ(proactive_signals);READ(publication_delay);READ(confirmation_increment);READ(max_confirmations);READ(max_peers);READ(max_per_group);
    READ(mitigate_confirmations);READ(mitigate_publication);READ(mitigate_pool);READ(mitigate_peers);
#undef READ
    validate(c);return c;
}
std::map<std::string,Reference> load_reference(const Config& c){
    if(c.mode=="off"||c.mode=="calibration"||c.detector=="fixed")return {};
    if(c.reference_file.empty())throw std::invalid_argument("Adaptive detection requires a prior honest reference_file");
    std::ifstream file(c.reference_file,std::ios::binary);
    if(!file)throw std::invalid_argument("Cannot open calibration reference");
    std::string text(1024*1024+1,'\0');file.read(text.data(),text.size());text.resize(static_cast<std::size_t>(file.gcount()));
    if(file.bad()||text.size()>1024*1024)throw std::invalid_argument("Invalid reference file size");
    const auto j=Json::parse(text,[](int depth,Json::parse_event_t,Json&){if(depth>16)throw std::invalid_argument("Reference nesting too deep");return true;});
    if((j.at("schema_version")!=1 && j.at("schema_version")!=2)||j.at("window_ticks")!=c.window_ticks||j.at("audit_ticks")!=c.audit_ticks)
        throw std::invalid_argument("Calibration window mismatch");
    std::map<std::string,Reference> result;
    for(auto it=j.at("metrics").begin();it!=j.at("metrics").end();++it){
        if(!it->at("count").is_number_unsigned())throw std::invalid_argument("Invalid reference sample count");
        result[it.key()]={it->at("count").get<std::uint64_t>(),it->at("mean").get<double>(),it->at("variance").get<double>()};
    }
    return result;
}
std::string reference_json(const Config& c,const Detector& d){
    Json metrics=Json::object();
    for(const auto& item:d.calibration()) if(item.second.count>=c.min_samples)
        metrics[item.first]={{"count",item.second.count},{"mean",item.second.mean},{"variance",item.second.variance}};
    return Json{{"schema_version",1},{"window_ticks",c.window_ticks},{"audit_ticks",c.audit_ticks},{"metrics",metrics}}.dump(2)+"\n";
}
}
