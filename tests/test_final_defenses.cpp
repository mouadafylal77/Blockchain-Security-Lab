#include "sim/simulator.hpp"
#include "detection/config_io.hpp"
#include "metrics/json_exporter.hpp"
#include "metrics/security_study.hpp"
#include <nlohmann/json.hpp>
#include <iostream>
#include <stdexcept>
using namespace bsl;
using Json=nlohmann::json;
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
template<class F>void rejects(F f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}check(caught,"Invalid hardened configuration accepted");}
void save(const sim::Simulator& s,const std::string& name){metrics::write_results(s,std::string(BSL_BINARY_DIR)+"/final-defenses/"+name);}
sim::Config config(const std::string& attack){return sim::load_config(std::string(BSL_SOURCE_DIR)+"/configs/final_defenses/"+attack+".json");}
void policy(){
    detection::Config c;c.mode="protected";c.hardened_security=true;c.hold_ticks=100;c.confirmation_ticks=1000;c.max_payment_hold_ticks=50;
    mitigation::Policy p(c);
    check(p.confirmations(0,2,0)==6 && p.payment_allowed(0,0),"Preventive floor must precede all observations, without suspension");
    detection::Alert a{10,0,"chain_instability","reorg_depth","node","fixed",2,.5};a.severity="critical";
    p.apply(a);const auto actions=p.actions;a.time=20;p.apply(a);
    check(p.actions==actions && p.payment_allowed(0,60) && p.confirmations(0,2,60)>6,"No renewal loop; bounded suspension independent from confirmations");
    a={100,0,"publication_anomaly","late_p95","producer:2","fixed",90,40};a.severity="persistent";a.evidence_sufficient=true;p.apply(a);
    check(p.delivery_delay(0,2,101)>0 && p.delivery_delay(0,1,101)==0,"Probation targets producer, not relay identity");
    a={100,1,"pool_deficit","share_deficit","pool_member","fixed",4,3};a.severity="persistent";
    p.apply(a);check(p.credit_share(1,101),"Unjustified deficit cannot sanction");a.evidence_sufficient=true;p.apply(a);
    check(!p.credit_share(1,101),"Sufficient persistent evidence activates escrow");
    p.review({{150,1,"share_deficit",4,"pool_member",true,200}});
    check(!p.credit_share(1,201),"Fresh sufficient deficit extends temporary escrow without counting another action");
    p.review({{210,1,"share_deficit",0,"pool_member",true,300}});
    check(p.credit_share(1,210),"Reassuring sufficient observations lift escrow");
    sim::MiningPool pool(2,{0,1},true);pool.share(1,false);pool.settle(10);pool.release(1);
    check(pool.shares[1]==1 && pool.escrowed[1]==0 && pool.released[1]==1 && pool.credit_ticks[1]==10,"Escrow returns real payout credits, with measured cost");
    detection::Config d;d.mode="attacked";d.hardened_security=true;d.cooldown_ticks=1;
    detection::Detector det(d);
    auto first=det.observe({{1,0,"late_p95",80,"producer:2",true}});
    auto next=det.observe({{2,0,"late_p95",80,"producer:2",true}});
    check(first.at(0).severity=="informative" && next.at(0).severity=="persistent","Public persistence classified explicitly");
    det.observe({{3,0,"late_p95",1,"producer:2",true}});
    check(det.observe({{4,0,"late_p95",80,"producer:2",true}}).at(0).severity=="informative","Normal sample resets persistence");
    rejects([]{detection::parse_config("{\"persistent_samples\":1}");});
    rejects([]{detection::parse_config("{\"pool_tail_probability\":0.5}");});
    rejects([]{detection::parse_config("{\"hardened_security\":true,\"preventive_confirmations\":101}");});
    // No fresh evidence: temporary restriction expires even without an exonerating block.
    a.time=300;p.apply(a);check(p.credit_share(1,400),"Escrow has finite expiry without new evidence");
    auto early=config("attack_51");early.nodes[0].hash_power=10;early.nodes[1].hash_power=45;early.nodes[2].hash_power=45;
    early.links={{0,1,40,0},{1,2,40,0},{0,2,40,0}};early.attack.confirmations=6;
    early.security.audit_ticks=10;early.security.cooldown_ticks=10;early.security.event_audits=true;early.security.proactive_signals=true;
    early.security.hold_ticks=1;early.security.confirmation_ticks=10000;
    sim::Simulator public_risk(early);public_risk.run();save(public_risk,"merchant_public_early");
    const auto merchant=Json::parse(metrics::study_json(public_risk)).at("merchant");
    check(public_risk.attack().accepted && merchant.at("actions_causally_before_acceptance").get<unsigned>()>0
        && public_risk.attack().accepted_required>6,"Public forks cause actual strengthening before acceptance");
}
void paired(const std::string& attack){
    for(const auto seed:{42ULL,43ULL}){
        auto c=config(attack);c.seed=seed;c.security.mode="attacked";
        sim::Simulator plain(c);plain.run();save(plain,attack+"_"+std::to_string(seed)+"_attacked");
        c.security.mode="protected";sim::Simulator protected_run(c);protected_run.run();save(protected_run,attack+"_"+std::to_string(seed)+"_protected");
        check(plain.metrics().blocks_produced==protected_run.metrics().blocks_produced,"Same horizon must preserve mining draws and total discoveries");
        const auto study=Json::parse(metrics::study_json(protected_run));
        if(attack=="attack_51"){
            check(protected_run.attack().accepted && protected_run.attack().accepted_required>=6,"Real merchant accepts only after preventive confirmations");
            check(protected_run.attack().accepted_at>plain.attack().accepted_at,"Preventive policy changes merchant timing");
            bool base_blocked=false;for(const auto& d:protected_run.merchant_decisions())if(d.base_ready && !d.eligible)base_blocked=true;
            check(base_blocked,"Public decisions prove causal delay before acceptance");
            for(const auto& e:protected_run.trace())if(e.type=="security_action")check(e.node==c.attack.target && e.time<protected_run.attack().accepted_at,"No useless confirmation actions on non-merchants or after public acceptance");
        }else if(attack=="selfish_mining"){
            check(protected_run.deferred_messages>0,"Producer probation must change actual delivery");
            for(const auto& d:protected_run.deferred_deliveries())check(d.producer==0,"Controlled reference must not delay honest-produced content through suspect relays");
        }else if(attack=="block_withholding"){
            check(protected_run.pool().held_ever[0]>0 && protected_run.pool().escrowed[0]>0,"Persistent sufficient deficit holds adversary payout credits");
            check(study.at("pool").at("adverse_payout_units").get<double>()<Json::parse(metrics::study_json(plain)).at("pool").at("adverse_payout_units").get<double>(),"Payout damage actually reduced in controlled pair");
            check(protected_run.pool().held_ever[1]==0,"Honest pool member remains credited in reference case");
            check(attacks::pool_reward(plain)==attacks::pool_reward(protected_run),"Escrow does not fabricate restored pool blocks");
        }else if(attack=="sybil"){
            check(plain.attack().success(plain) && !protected_run.attack().success(protected_run),"Available public peers operationally break complete eclipse");
            check(protected_run.reconnections>0 && protected_run.nodes()[c.attack.target].chain.known_count()>1,"Proactive connections carry real blockchain data");
            const auto& victim=study.at("sybil").at(c.attack.target);
            check(victim.at("initial_eclipse_ticks")==0 && victim.at("initial_eclipse_censored")==false,"Inbound and outbound independent connections both end initial eclipse");
        }
        c.attack.enabled=false;c.security.mode="baseline";sim::Simulator honest(c);honest.run();save(honest,attack+"_"+std::to_string(seed)+"_honest");
        c.security.mode="protected";sim::Simulator honest_protected(c);honest_protected.run();save(honest_protected,attack+"_"+std::to_string(seed)+"_honest_protected");
        if(attack=="attack_51")check(honest_protected.attack().accepted,"Honest merchant must not remain permanently blocked");
        if(attack=="block_withholding")check(honest_protected.pool().held_ever[0]+honest_protected.pool().held_ever[1]==0,"No honest sanctions in fixed reference seeds");
        if(seed==42){sim::Simulator repeat(c);repeat.run();check(metrics::results_json(repeat)==metrics::results_json(honest_protected),"Hardened policy is reproducible");}
    }
    if(attack=="sybil"){
        auto c=config(attack);c.security.restrict_bootstrap=true;c.security.bootstrap_peers=c.attack.sybil_identities;
        sim::Simulator restricted(c);restricted.run();save(restricted,"sybil_restricted");
        check(restricted.attack().success(restricted),"Public directory of controlled peers cannot guarantee recovery");
    }
}
int main(int argc,char** argv){try{
    if(argc!=2)throw std::invalid_argument("Expected case");const std::string name=argv[1];
    if(name=="policy")policy();else if(name=="merchant")paired("attack_51");else if(name=="selfish")paired("selfish_mining");
    else if(name=="pool")paired("block_withholding");else if(name=="sybil")paired("sybil");else throw std::invalid_argument("Unknown case");
    std::cout<<"PASS "<<name<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
