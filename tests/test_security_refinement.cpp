#include "sim/simulator.hpp"
#include "metrics/json_exporter.hpp"
#include "metrics/security_study.hpp"
#include "detection/config_io.hpp"
#include <nlohmann/json.hpp>
#include <iostream>
#include <stdexcept>
using namespace bsl;
using Json=nlohmann::json;
void check(bool v,const char* msg){if(!v)throw std::runtime_error(msg);}
template<class F>void rejects(F f){bool failed=false;try{f();}catch(const std::exception&){failed=true;}check(failed,"Invalid configuration must fail");}
void save(const sim::Simulator& s,const std::string& name){metrics::write_results(s,std::string(BSL_BINARY_DIR)+"/security-refinement/"+name);}
int main(int argc,char** argv){try{
    if(argc!=2)throw std::invalid_argument("Expected case");const std::string test=argv[1];
    if(test=="merchant"){
        detection::Config p;p.mode="protected";p.hold_ticks=500;p.confirmation_ticks=1000;p.max_payment_hold_ticks=200;
        mitigation::Policy cap(p);detection::Alert a{10,0,"chain_instability","reorg_depth","node","fixed",2,.5};
        cap.apply(a);a.time=100;cap.apply(a);
        check(cap.hold_until(0)==210 && cap.payment_allowed(0,210) && cap.confirmations(0,2,210)==7,"Repeated warnings cannot exceed explicit hold cap; confirmations persist");
        a.time=2000;cap.apply(a);check(cap.hold_until(0)==2200,"Independent risk episode opens after expiry");
        p.hold_ticks=0;mitigation::Policy pure(p);pure.apply(a);check(pure.payment_allowed(0,2000) && pure.confirmations(0,2,2000)==7,"Confirmation-only policy has no payment suspension");
        auto c=sim::load_config(std::string(BSL_SOURCE_DIR)+"/configs/attack_51.json");
        c.attack.public_notice=true;c.nodes[0].hash_power=10;c.nodes[1].hash_power=45;c.nodes[2].hash_power=45;
        c.links={{0,1,40,0},{1,2,40,0},{0,2,40,0}};c.attack.confirmations=6;
        c.security.mode="protected";c.security.audit_ticks=10;c.security.event_audits=true;
        c.security.hold_ticks=1;c.security.confirmation_ticks=1000000;
        sim::Simulator early(c);early.run();
        check(early.attack().accepted && early.attack().accepted_required>6,"Public forks lead to actual elevated-confirmation acceptance");
        bool blocked=false,causal=false;
        for(const auto& d:early.merchant_decisions()){
            if(d.base_ready && !d.eligible)blocked=true;
            if(d.time==early.attack().accepted_at && d.eligible && d.required==early.attack().accepted_required)causal=true;
        }
        check(blocked && causal,"Public decision ledger explains withheld and accepted opportunities");save(early,"merchant_early");
        c.attack.enabled=false;c.security.proactive_signals=true;c.security.thresholds["silence_ticks"]=1;c.security.cooldown_ticks=1;
        c.security.hold_ticks=c.security.confirmation_ticks=1000000;
        sim::Simulator refused(c);refused.run();check(!refused.attack().accepted,"Repeated honest warnings can refuse all payments");save(refused,"honest_unbounded");
        c.security.max_payment_hold_ticks=50;sim::Simulator bounded(c);bounded.run();
        check(bounded.attack().accepted,"Hold cap improves availability in controlled honest stress case");save(bounded,"honest_bounded");
        c.security.mode="attacked";sim::Simulator honest(c);honest.run();check(honest.attack().accepted && honest.defenses().actions==0,"Honest no-defense control");save(honest,"honest_control");
        auto late=sim::load_config(std::string(BSL_SOURCE_DIR)+"/configs/attack_51.json");late.attack.public_notice=true;late.security.mode="protected";late.security.event_audits=true;
        sim::Simulator s(late);s.run();check(s.attack().success(s),"No favorable rewrite: late policy can still lose payment");
        auto m=Json::parse(metrics::study_json(s)).at("merchant");
        check(m.at("actions_strictly_before_acceptance")==0 && m.at("actions_causally_before_acceptance")==0 && m.at("double_spend")==true,"Late warnings never count as timely defense");
        check(m.at("acceptance_event").at("time")==s.attack().accepted_at,"Acceptance trace/event sequence exported");save(s,"merchant_late");
        sim::Simulator repeat(late);repeat.run();check(metrics::results_json(s)==metrics::results_json(repeat),"Ledger instrumentation reproduces exactly");
        rejects([]{detection::parse_config("{\"max_payment_hold_ticks\":-1}");});
    }else throw std::invalid_argument("Unknown case");
    std::cout<<"PASS "<<test<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
