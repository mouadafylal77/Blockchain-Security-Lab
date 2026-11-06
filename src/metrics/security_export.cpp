#include "metrics/security_export.hpp"
#include "metrics/detection_evaluation.hpp"
#include "metrics/security_study.hpp"
#include "sim/simulator.hpp"
#include <nlohmann/json.hpp>
namespace bsl::metrics {
std::string security_json(const sim::Simulator& sim){
    using Json=nlohmann::json;
    const auto& c=sim.config().security;
    if(c.mode=="off")return Json{{"mode","off"},{"evaluation",nullptr}}.dump();
    if(c.mode=="calibration")return Json{{"mode","calibration"},{"evaluation",nullptr},{"audits",sim.security_audits},{"feature_evaluations",sim.detector().operations()}}.dump();
    Json alerts=Json::array();
    Json reference=Json::object();for(const auto& r:sim.detector().reference())reference[r.first]={{"count",r.second.count},{"mean",r.second.mean},{"variance",r.second.variance}};
    Json peers=Json::array();for(std::size_t i=0;i<sim.nodes().size();++i){Json ids=Json::array();for(const auto& p:sim.network().peers(i))ids.push_back(sim.nodes()[p.node].id);peers.push_back({{"node",sim.nodes()[i].id},{"peers",ids}});}
    for(const auto& a:sim.detector().alerts())alerts.push_back({{"time",a.time},{"observer",sim.nodes().at(a.node).id},
        {"node_index",a.node},{"component",a.component},{"behavior",a.behavior},{"metric",a.metric},
        {"value",a.value},{"threshold",a.threshold},{"detector",a.detector},{"phase",sim.audit_phase(a.time)},
        {"severity",a.severity},{"evidence_sufficient",a.evidence_sufficient},{"public_samples",a.samples},
        {"operational",sim.audit_phase(a.time)!="final_observation"}});
    // Evaluation only: ground truth never enters the detector or mitigation policy.
    std::string truth;const auto& attack=sim.config().attack;
    if(attack.enabled){if(attack.type=="51")truth="chain_instability";else if(attack.type=="selfish")truth="publication_anomaly";
        else if(attack.type=="withholding")truth="pool_deficit";else if(attack.type=="sybil")truth="connectivity_anomaly";}
    const auto cf=evaluate(sim.detector().alerts(),truth);
    Json latency=nullptr;
    for(const auto& a:sim.detector().alerts())if(a.behavior==truth){latency=a.time;break;}
    const double payment_loss=attack.type=="51"&&sim.attack().success(sim)?1:0;
    return Json{{"policy_revision",c.hardened_security?"hardened_m4_v1":"legacy_m4"},{"study",Json::parse(study_json(sim))},{"mode",c.mode},{"detector",c.detector},{"alerts",alerts},{"final_peers",peers},{"initial_reference",reference},
        {"economic",{{"payment_loss_coin_units",payment_loss},{"pool_reward_units",attacks::pool_reward(sim)},
            {"unit_definition","One synthetic coin lost on accepted reversal; one unit per pool block active at target. Counterfactual losses require paired control."}}},
        {"evaluation",{{"unit","run x behavior family; exposure begins at tick 0"},{"ground_truth_family",truth},
            {"tp",cf.tp},{"fp",cf.fp},{"tn",cf.tn},{"fn",cf.fn},{"precision",cf.precision()},
            {"recall",cf.recall()},{"f1",cf.f1()},{"latency_ticks",latency},
            {"warning","Run-level association is not causal identification; failed attempts remain exposed"}}},
        {"overhead",{{"audits",sim.security_audits},{"feature_evaluations",sim.detector().operations()},
            {"uncalibrated_features_skipped",sim.detector().insufficient_samples()},
            {"peak_observation_samples",sim.peak_observation_samples},{"mitigation_actions",sim.defenses().actions},
            {"payment_checks",sim.defenses().payment_checks},{"payment_deferrals",sim.defenses().payment_deferrals},
            {"deferred_messages",sim.deferred_messages},{"deferred_ticks",sim.deferred_ticks},
            {"shares_uncredited",sim.pool().shares_uncredited},
            {"reconnections",sim.reconnections},{"sync_messages",sim.sync_messages},
            {"real_cpu_time_measured",false}}}}.dump();
}
}
