#include "sim/simulator.hpp"
#include "metrics/json_exporter.hpp"
#include "metrics/security_study.hpp"
#include <nlohmann/json.hpp>
#include <iostream>
#include <stdexcept>
using namespace bsl;using Json=nlohmann::json;
void check(bool v,const char* s){if(!v)throw std::runtime_error(s);}
sim::Config input(const std::string& name){return sim::load_config(std::string(BSL_SOURCE_DIR)+"/configs/m4/"+name+"_fixed_protected.json");}
void save(const sim::Simulator& s,const std::string& name){metrics::write_results(s,std::string(BSL_BINARY_DIR)+"/security-refinement/"+name);}
int main(int argc,char** argv){try{
    if(argc!=2)throw std::invalid_argument("Expected case");const std::string name=argv[1];
    if(name=="probation"){
        auto c=input("selfish_mining");sim::Simulator protected_run(c);protected_run.run();
        const auto cost=Json::parse(metrics::study_json(protected_run)).at("probation");
        check(cost.at("honest_sender_messages").get<unsigned>()>0 && cost.at("honest_producer_messages").get<unsigned>()>0,"Probation can delay honest relays and honest payloads");
        check(cost.at("messages")==protected_run.deferred_messages,"Detailed accounting matches operational count");save(protected_run,"probation_protected");
        c.security.mode="attacked";sim::Simulator none(c);none.run();check(none.deferred_messages==0,"No hidden probation without mitigation");save(none,"probation_attacked");
    }else if(name=="pool"){
        auto c=input("block_withholding");sim::Simulator protected_run(c);protected_run.run();
        const auto cost=Json::parse(metrics::study_json(protected_run)).at("pool");
        check(cost.at("adverse_uncredited").get<unsigned>()>0 && cost.at("honest_uncredited")==0,"Credit losses separated by role in controlled case");save(protected_run,"pool_protected");
        c.security.mode="attacked";sim::Simulator attacked(c);attacked.run();save(attacked,"pool_attacked");
        c.attack.enabled=false;c.security.mode="baseline";sim::Simulator honest(c);honest.run();save(honest,"pool_honest");
        check(attacks::pool_reward(honest)>attacks::pool_reward(attacked),"Paired honest pool reward measures actual lost units");
        c.security.mode="protected";c.security.min_shares=1;c.security.thresholds["share_deficit"]=.01;
        sim::Simulator false_sanction(c);false_sanction.run();
        auto stress=Json::parse(metrics::study_json(false_sanction)).at("pool");
        check(stress.at("honest_uncredited").get<unsigned>()>0,"Deliberately sensitive synthetic test records honest sanctions, never hides them");save(false_sanction,"pool_honest_stress");
    }else if(name=="bootstrap"){
        auto c=input("sybil");c.security.restrict_bootstrap=true;c.security.bootstrap_peers={1};
        sim::Simulator available(c);available.run();check(!available.attack().success(available),"One available independent transmitting peer can recover this scenario");save(available,"sybil_available");
        c.security.bootstrap_delay_ticks=250;sim::Simulator delayed(c);delayed.run();
        const auto victim=Json::parse(metrics::study_json(delayed)).at("sybil").at(c.attack.target);
        check(!victim.at("recovery_from_request_ticks").is_null() && victim.at("recovery_from_request_ticks").get<unsigned>()>=251,"Bootstrap delay must have real operational effect");save(delayed,"sybil_delayed");
        c.security.bootstrap_delay_ticks=0;c.security.bootstrap_peers=c.attack.sybil_identities;
        sim::Simulator unavailable(c);unavailable.run();save(unavailable,"sybil_unavailable");
        const auto missing=Json::parse(metrics::study_json(unavailable)).at("sybil").at(c.attack.target);
        check(unavailable.attack().success(unavailable) && missing.at("first_adoption_at").is_null(),"No omniscient recovery through inaccessible independent peers");
        c.security.bootstrap_peers={999};bool failed=false;try{sim::Simulator invalid(c);}catch(const std::exception&){failed=true;}check(failed,"Invalid public directory rejected");
    }else throw std::invalid_argument("Unknown case");
    std::cout<<"PASS "<<name<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
