#include "sim/simulator.hpp"
#include "detection/config_io.hpp"
#include "metrics/json_exporter.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace bsl;
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
int main(int argc,char** argv){try{
    if(argc!=2)throw std::invalid_argument("Expected test");
    const std::string test=argv[1];
    if(test=="confirmations"){
        detection::Config c;c.mode="protected";
        mitigation::Policy p(c);
        p.apply({10,0,"chain_instability","reorg_depth","node","fixed",2,0.5});
        check(!p.payment_allowed(0,11) && p.confirmations(0,2,11)==7,"Alert raises confirmations and gates new settlements");
        check(p.payment_allowed(0,510) && p.confirmations(0,2,510)==7,"Hold expires but elevated threshold persists");
        check(p.confirmations(0,2,1010)==2,"Elevated threshold expires independently");
        auto config=sim::load_config(std::string(BSL_SOURCE_DIR)+"/configs/attack_51.json");
        config.nodes[0].hash_power=10;config.nodes[1].hash_power=45;config.nodes[2].hash_power=45;
        config.links={{0,1,40,0},{1,2,40,0},{0,2,40,0}};
        config.attack.confirmations=10;
        config.security=c;config.security.audit_ticks=10;
        sim::Simulator protected_run(config);protected_run.run();
        check(protected_run.defenses().actions>0,"Observed forks must operationally activate merchant hold");
        config.security.mode="attacked";
        sim::Simulator control(config);control.run();
        check(control.attack().accepted,"Unprotected merchant accepts");
        check(!protected_run.attack().accepted || protected_run.attack().accepted_at>control.attack().accepted_at,"Protected merchant settlement actually changes");
    }else if(test=="configuration"){
        for(const auto& text:{"{\"audit_ticks\":0}","{\"alpha\":1.1}","{\"window_ticks\":-1}","{\"hold_ticks\":true}","{\"unknown\":1}","{\"thresholds\":{\"attack_label\":1}}","{\"max_peers\":0}"}){
            bool rejected=false;try{detection::parse_config(text);}catch(const std::exception&){rejected=true;}check(rejected,"Invalid security JSON must fail");
        }
        auto c=sim::load_config(std::string(BSL_SOURCE_DIR)+"/configs/attack_51.json");c.security.mode="calibration";
        bool rejected=false;try{sim::Simulator invalid(c);}catch(const std::exception&){rejected=true;}check(rejected,"Active attack cannot be declared an honest calibration run");
    }else if(test=="peers"){
        auto c=sim::load_config(std::string(BSL_SOURCE_DIR)+"/configs/sybil.json");
        c.security.mode="protected";c.security.mitigate_publication=false;
        c.nodes[3].peer_group="network-c";c.nodes[4].peer_group="network-c";
        sim::Simulator s(c);s.run();
        check(s.reconnections>0 && s.sync_messages>0 && s.nodes()[c.attack.target].chain.known_count()>1,"Reconnect and catch-up must deliver actual blocks to eclipsed victim");
        check(!s.attack().success(s),"Reconnection defeats complete isolation in this controlled case");
        std::map<std::string,unsigned> groups;
        for(const auto& p:s.network().peers(c.attack.target)){const auto& n=c.nodes[p.node];++groups[n.peer_group.empty()?n.id:n.peer_group];}
        for(const auto& group:groups)check(group.second<=c.security.max_per_group,"Local group admission cap");
        c.security.mode="attacked";sim::Simulator unprotected(c);unprotected.run();
        check(unprotected.attack().success(unprotected)&&unprotected.reconnections==0,"No hidden repair in attacked mode");
    }else if(test=="pool"){
        auto c=sim::load_config(std::string(BSL_SOURCE_DIR)+"/configs/block_withholding.json");
        c.security.mode="protected";
        c.security.mitigate_publication=false;c.security.mitigate_peers=false;c.security.mitigate_confirmations=false;
        sim::Simulator insufficient(c);insufficient.run();
        bool pool_alarm=false;for(const auto& a:insufficient.detector().alerts())pool_alarm|=a.behavior=="pool_deficit";
        check(insufficient.pool().full_withheld>0&&!pool_alarm&&insufficient.pool().shares_uncredited==0,"Short windows can miss an active withholding attack");
        c.security.window_ticks=10000;
        sim::Simulator s(c);s.run();
        check(s.pool().shares_uncredited>0 && s.pool().shares[c.attack.adversary]<s.pool().observed_shares[c.attack.adversary],"Audit must actually change credited shares");
        check(s.pool().submitted_full[c.attack.adversary]==0 && s.pool().full_withheld>0,"Detector ledger must exclude private full solutions");
        c.security.mode="attacked";sim::Simulator no_defense(c);no_defense.run();
        check(no_defense.pool().shares_uncredited==0 && no_defense.pool().shares[c.attack.adversary]>s.pool().shares[c.attack.adversary],"Unprotected payout ledger remains unchanged");
        mitigation::Policy policy(c.security);
        policy.apply({10,0,"pool_deficit","share_deficit","pool_member","fixed",4,3});
        check(policy.credit_share(0,11),"Observation-only mode never suspends shares");
    }else if(test=="publication"){
        auto c=sim::load_config(std::string(BSL_SOURCE_DIR)+"/configs/selfish_mining.json");
        c.security.mode="protected";c.security.audit_ticks=20;
        c.security.publication_delay=100;
        sim::Simulator s(c);s.run();
        check(s.deferred_messages>0 && s.deferred_ticks>=c.security.publication_delay,"Probation must enqueue actual delayed messages");
        bool released=false;
        for(const auto& delayed:s.trace())if(delayed.type=="probation_delay"){
            for(const auto& release:s.trace())if(release.type=="delayed_delivery" && release.node==delayed.node && release.hash==delayed.hash && release.time>=delayed.time+delayed.depth)released=true;
        }
        check(released,"Held payload must really arrive later, preserving eventual validation");
        c.security.mode="attacked";sim::Simulator no_defense(c);no_defense.run();
        check(no_defense.deferred_messages==0,"Detection alone must not delay messages");
    }else if(test=="purity"){
        detection::Config c;c.mode="attacked";
        const std::vector<detection::Feature> observed{{100,2,"reorg_depth",3,"node"},{100,2,"late_p95",100,"peer:1"}};
        detection::Detector a(c),b(c);
        const auto x=a.observe(observed),y=b.observe(observed);
        check(x.size()==y.size()&&x[0].threshold==y[0].threshold,"Identical public observations yield identical alerts regardless of labels");
        c.mode="baseline";detection::Detector honest_label(c);const auto z=honest_label.observe(observed);
        c.mode="protected";detection::Detector protected_label(c);const auto q=protected_label.observe(observed);
        check(z.size()==x.size()&&q.size()==x.size()&&z[0].value==x[0].value&&q[0].threshold==x[0].threshold,
            "Exposure and protection labels cannot change a detector decision for the same observations");
        auto config=sim::load_config(std::string(BSL_SOURCE_DIR)+"/configs/baseline.json");
        config.nodes[0].hash_power=100;config.nodes[1].hash_power=0;
        config.security.mode="attacked";
        config.security.thresholds["silence_ticks"]=100000;
        sim::Simulator honest(config);honest.run();
        check(honest.detector().alerts().empty(),"Quiet honest reference must not trigger fixed alerts");
        config.security.mode="calibration";config.security.detector="adaptive";config.seed=7;
        sim::Simulator reference(config);reference.run();
        const std::string path=std::string(BSL_BINARY_DIR)+"/test-quiet-reference.json";
        {std::ofstream file(path);file<<detection::reference_json(config.security,reference.detector());}
        config.security.mode="baseline";config.security.reference_file=path;config.seed=42;
        sim::Simulator adaptive_honest(config);adaptive_honest.run();
        check(adaptive_honest.detector().alerts().empty(),"Quiet independent honest run must not trigger adaptive alerts");
    }else if(test=="calibration"){
        auto config=sim::load_config(std::string(BSL_SOURCE_DIR)+"/configs/selfish_mining_control.json");
        config.security.mode="calibration";config.security.detector="adaptive";
        sim::Simulator baseline(config);baseline.run();
        const std::string path=std::string(BSL_BINARY_DIR)+"/test-calibration.json";
        {std::ofstream file(path);file<<detection::reference_json(config.security,baseline.detector());}
        config.security.mode="attacked";config.security.reference_file=path;
        sim::Simulator replay(config);replay.run();
        sim::Simulator repeat(config);repeat.run();
        check(metrics::results_json(replay)==metrics::results_json(repeat),"Security run reproducibility");
        bool rejected=false;
        try{config.security.reference_file="does-not-exist.json";sim::Simulator invalid(config);}catch(const std::exception&){rejected=true;}
        check(rejected,"Missing reference rejected");
    }else throw std::invalid_argument("Unknown test");
    std::cout<<"PASS "<<test<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
