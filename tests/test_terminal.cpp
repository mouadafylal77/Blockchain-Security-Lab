#include "terminal/interactive.hpp"
#include "terminal/simple.hpp"
#include "sim/simulator.hpp"
#include "metrics/json_exporter.hpp"
#include "detection/config_io.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <set>
#include <cmath>
using namespace bsl;
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
template<class F>void reject(F f){bool refused=false;try{f();}catch(const std::exception&){refused=true;}check(refused,"Invalid input must be rejected");}
sim::Config form(const std::string& text,std::string* transcript=nullptr){
    std::istringstream input(text);std::ostringstream output;auto c=terminal::configure(input,output);if(transcript)*transcript=output.str();return c;
}
std::string new_output(const std::string& name){
    for(unsigned n=0;;++n){auto p=std::filesystem::path(BSL_BINARY_DIR)/"terminal-results"/(name+std::to_string(n));if(!std::filesystem::exists(p))return p.string();}
}
std::string simple_session(int attack,bool defended,bool invalid=false){
    std::ostringstream in;
    in<<"1\n5\n2\n30\n"<<attack<<'\n';
    if(attack!=4){if(invalid)in<<"-1\n";in<<"40\n";}
    else if(invalid)in<<"9\n";
    in<<(defended?1:0)<<"\n1\n";
    return in.str();
}
std::string custom_session(int attack){
    std::ostringstream in;
    in<<"2\n5\n2\n20\n"<<attack<<"\n1\n42\n40\n";
    if(attack!=4)in<<"1\n";
    in<<"60\n";if(attack!=4)in<<"0\n";
    in<<"4\n3\n7\n3\n"; // target, line, link latency and jitter
    if(attack==1)in<<"6\n";
    if(attack==3)in<<"1\n"; // honest member joins pool
    if(attack==4)in<<"1\n0\n0\n0\n1\n2\n3\n4\n"; // one Sybil, no capture, five groups
    in<<(attack==3?200:50)<<'\n'<<(attack==3?2:1)<<"\n1000000\n100000\n100\n100000\n100000\n1\n";
    in<<(attack==3?10000:2000)<<"\n10\n500\n";
    if(attack==1)in<<"0.5\n0.005\n500\n";
    if(attack==2)in<<"40\n0.25\n0.005\n";
    if(attack==3)in<<"3\n64\n";
    if(attack==4)in<<"0.8\n500\n";
    in<<"500\n";
    if(attack==1)in<<"2000\n3\n100\n0\n1\n";
    if(attack==2)in<<"100\n";
    if(attack==4)in<<"4\n1\n";
    in<<"0\n0\n1\n";return in.str();
}
int main(int argc,char** argv){try{
    if(argc!=2)throw std::invalid_argument("Expected case");std::string name=argv[1];
    if(name=="simple"){
        auto c=form("1\n4\n2\n8\n1\n40\n1\n1\n");check(c.target_height==8 && c.nodes.size()==4 && c.attack.type=="51" && c.security.mode=="protected","Simple wizard choices");
        check(c.attack.public_notice && c.security.event_audits,"New scenarios use public notice and fast audits");
        check(c.security.hardened_security,"Simple fixed defense enables hardened policy");
        check(!form("1\n4\n2\n8\n1\n40\n0\n1\n").security.hardened_security,"Simple unprotected mode disables hardened policy");
        check(form("1\n4\n2\n8\n1\n40\n2\n1\n").security.hardened_security,"Simple adaptive defense enables hardened policy");
    }else if(name=="custom"){
        std::string transcript;
        auto c=form("2\n4\n3\n12\n3\n1\n123\n20\n1\n30\n1\n50\n0\n3\n3\n20\n7\n1\n"+std::string(40,'\n'),&transcript);
        check(c.seed==123 && c.nodes[0].hash_power==20 && c.nodes[1].hash_power==30 && c.nodes[2].hash_power==50,"Custom resources must be individual actual weights");
        check(!c.security.hardened_security,"Custom policy unchanged");
        check(c.attack.adversaries==std::vector<std::size_t>({0,1}) && c.attack.pool_members.size()==3,"Coordinated adversary and pool membership");
        check(c.links.size()==3 && c.links[0].latency==20 && c.links[0].jitter==7,"Custom network");
        check(transcript.find("Confirmations marchandes")==std::string::npos,"No irrelevant merchant questions for pool");
        sim::Simulator s(c);s.run();check(s.pool().full_withheld>0 && s.pool().submitted_full[0]==0 && s.pool().submitted_full[1]==0,"Both adversarial pool miners withhold real solutions");
    }else if(name=="validation"){
        auto c=form("1\nabc\n1\n4\n2\nb\n3\n9\n5\n0\n0\n1\n");
        check(c.nodes[2].hash_power>0 && c.target_height==9,"Invalid inputs and back navigation");
        terminal::Request bad;bad.miners=5;reject([&]{terminal::generate(bad);});
        bad.miners=2;bad.attack="sybil";bad.nodes=3;reject([&]{terminal::generate(bad);});
        c.attack.type="51";c.attack.adversaries={0,0};reject([&]{sim::validate_config(c);});
        reject([&]{detection::parse_config("{\"hold_ticks\":500,\"confirmation_ticks\":100}");});
        reject([&]{metrics::validate_output_directory("results/raw/m5_30seeds/evaluation/test");});
        reject([&]{metrics::validate_output_directory("build/m5-reference");});
        std::istringstream cancel("q\n");std::ostringstream out;check(terminal::interactive(cancel,out)==0,"Cancel cleanly");
    }else if(name=="replay"){
        terminal::Request r;r.attack="selfish";r.height=20;
        auto c=terminal::generate(r);sim::Simulator a(c),b(sim::parse_config(sim::config_json(c)));a.run();b.run();
        check(metrics::results_json(a)==metrics::results_json(b),"Generated JSON replay byte-identical");
        r.seed=43;sim::Simulator other(terminal::generate(r));other.run();
        check(a.trace().front().time!=other.trace().front().time || a.trace().front().hash!=other.trace().front().hash,"Different seeds change probabilistic discoveries");
        const auto dir=new_output("replay");std::ostringstream out;terminal::execute(c,dir,out,false);
        auto replay=sim::load_config(dir+"/config.json");sim::Simulator again(replay);again.run();
        check(metrics::results_json(a)==metrics::results_json(again),"Exported config preserves all modeled results");
        reject([&]{terminal::execute(c,dir,out,false);});
    }else if(name=="attacks"){
        for(const auto* type:{"51","selfish","withholding","sybil"}){
            terminal::Request r;r.nodes=5;r.miners=2;r.height=15;r.attack=type;r.defenses=true;
            auto c=terminal::generate(r);sim::Simulator s(c);s.run();
            check(s.metrics().blocks_produced>0,"Every generated attack executes actual discoveries");
            if(c.attack.type=="sybil"){for(auto id:c.attack.sybil_identities)check(c.nodes[id].hash_power==0,"No hash power from Sybil identities");check(s.reconnections>0,"Sybil mitigation operational");}
            if(c.attack.type=="withholding")check(s.pool().observed_shares[0]>=s.pool().full_withheld,"Shares include full solutions");
        }
    }else if(name=="adaptive"){
        terminal::Request r;r.height=8;r.attack="51";r.defenses=true;
        auto c=terminal::generate(r);c.security.detector="adaptive";
        const auto dir=new_output("adaptive");std::ostringstream output;terminal::execute(c,dir,output,false);
        const auto replay=sim::load_config(dir+"/config.json");sim::Simulator s(replay);s.run();
        check(!s.detector().reference().empty(),"Prior honest calibration actually loaded");
        std::ifstream file(dir+"/results.json");std::stringstream content;content<<file.rdbuf();
        check(content.str()==metrics::results_json(s),"Adaptive export replays exactly with saved reference");
        check(output.str().find("Calibration honnete separee")!=std::string::npos,"Calibration is disclosed");
        c.security.min_samples=100000;
        reject([&]{terminal::execute(c,new_output("insufficient"),output,false);});
    }else if(name=="target"){
        terminal::Request r;r.height=5;auto c=terminal::generate(r);c.security.mode="off";
        sim::Simulator s(c);s.run();check(s.target_reached() && s.stop_reason()=="target_height_drained","Stop production on local target then drain");
        check(s.nodes()[c.observer].chain.tip().index>=5 && s.metrics().blocks_produced<c.max_blocks,"Height is not discovery count");
        c.duration_ticks=1;sim::Simulator limited(c);limited.run();check(!limited.target_reached(),"Hard horizon does not invent target achievement");
        c=sim::load_config(std::string(BSL_SOURCE_DIR)+"/configs/fork_reorg.json");sim::Simulator forks(c);forks.run();
        check(forks.metrics().forks>0 && forks.metrics().reorganizations>0,"Original probabilistic forks and consensus remain operational");
    }else if(name=="policy"){
        detection::Config c;c.mode="protected";c.hold_ticks=10;c.confirmation_ticks=100;
        mitigation::Policy p(c);p.apply({20,0,"chain_instability","reorg_depth","node","fixed",2,.5});
        check(!p.payment_allowed(0,29) && p.confirmations(0,2,29)==7,"Initial hold");
        check(p.payment_allowed(0,30) && p.confirmations(0,2,30)==7,"Elevated threshold really applies after hold");
        check(p.confirmations(0,2,120)==2,"Independent expiry boundary");
        check(p.confirmations(0,200,31)==200,"Never lower original merchant rule");
        c.proactive_signals=true;mitigation::Policy early(c);early.apply({5,0,"connectivity_anomaly","silence_ticks","peers","fixed",10,5});
        check(!early.payment_allowed(0,6),"Only optional public silence warning can proactively hold");
    }else if(name=="timing"){
        auto c=sim::load_config(std::string(BSL_SOURCE_DIR)+"/configs/attack_51.json");
        c.attack.public_notice=true;c.security.mode="protected";c.security.event_audits=true;
        sim::Simulator late(c);late.run();check(late.attack().accepted && late.attack().success(late),"Late reaction cannot undo accepted payment");
        for(const auto& a:late.detector().alerts())if(a.node==c.attack.target && a.behavior=="chain_instability")check(a.time>=late.attack().accepted_at,"Private chain never triggers premature reorg alert");
        c.nodes[0].hash_power=10;c.nodes[1].hash_power=45;c.nodes[2].hash_power=45;
        c.links={{0,1,40,0},{1,2,40,0},{0,2,40,0}};c.attack.confirmations=10;c.security.audit_ticks=10;
        sim::Simulator early(c);early.run();bool prior=false;
        for(const auto& a:early.detector().alerts())if(a.node==c.attack.target && (!early.attack().accepted || a.time<early.attack().accepted_at))prior=true;
        check(prior && early.defenses().actions>0,"Public forks can trigger policy before settlement");
        auto persistent=c;persistent.attack.confirmations=6;persistent.security.hold_ticks=1;persistent.security.confirmation_ticks=1000000;
        sim::Simulator strengthened(persistent);strengthened.run();
        check(strengthened.attack().accepted && strengthened.attack().accepted_required>persistent.attack.confirmations,
            "Merchant actually accepts under elevated confirmations after the hold");
        c.attack.enabled=false;c.security.proactive_signals=true;c.security.thresholds["silence_ticks"]=1;
        c.security.hold_ticks=c.security.confirmation_ticks=1000000;c.security.cooldown_ticks=1;
        sim::Simulator honest(c);honest.run();check(!honest.attack().accepted && honest.defenses().actions>0,"Honest merchant can remain unaccepted from false holds");
        c.security.mode="attacked";sim::Simulator honest_control(c);honest_control.run();check(honest_control.attack().accepted,"Same honest payment is accepted without false sanctions");
        c.attack.enabled=true;c.security.mode="protected";sim::Simulator blocked(c);blocked.run();
        check(!blocked.attack().accepted && blocked.outcome()=="inconclusive","Zero acceptances are not successful protection");
    }else if(name=="notice"){
        auto c=sim::load_config(std::string(BSL_SOURCE_DIR)+"/configs/attack_51.json");c.attack.public_notice=true;
        sim::Simulator s(c);s.run();sim::Time notice=0,publication=0;
        for(const auto& e:s.trace()){
            if(e.type=="payment_notice" && e.node==c.attack.adversary && !notice)notice=e.time;
            if(e.type=="publication" && !publication)publication=e.time;
        }
        check(notice>s.attack().accepted_at && publication>=notice,"Publication waits for propagated public merchant notice");
        check(s.attack().accepted_confirmations>=s.attack().accepted_required,"Observed acceptance threshold recorded");
        auto changed=c;changed.links.clear();sim::Simulator isolated(changed);isolated.run();
        check(!isolated.attack().notified && !isolated.attack().published,"No public notification path, no omniscient publication");
        c.nodes[0].hash_power=0;sim::Simulator powerless(c);powerless.run();
        check(powerless.attack().accepted && powerless.outcome()=="failed","Zero-power attack can genuinely fail while honest payment completes");
    }else if(name=="resources"){
        terminal::Request r;r.nodes=30;r.miners=10;r.height=15;r.attack="51";r.adversary_percent=40;
        auto c=terminal::generate(r);double sum=0;for(const auto& n:c.nodes)sum+=n.hash_power;
        check(std::abs(sum-100)<1e-10 && c.nodes[0].hash_power==40,"Explicit 40/60 split without double counting");
        r.adversary_percent=10;check(terminal::generate(r).nodes[0].hash_power==10,"Sub-majority attempts allowed");
        r.adversary_percent=101;reject([&]{terminal::generate(r);});
        r.adversary_percent=40;r.miners=1;reject([&]{terminal::generate(r);});
        r.miners=10;r.attack="withholding";r.pool_honest_miners=10;reject([&]{terminal::generate(r);});
        r.attack="sybil";r.sybil_count=30;reject([&]{terminal::generate(r);});
    }else if(name=="presentation"){
        std::string out;
        auto c=form("?\n1\n5\n2\n15\n1\n40\n1\n2\n6\n30\n1\n1\n",&out);
        check(c.nodes[0].hash_power==30,"Recap allows selecting and correcting earlier power question");
        check(out.find("Recapitulatif")!=std::string::npos && out.find("Valeur proposee")!=std::string::npos,"Readable recap and on-demand help");
        check(out.find("[1] (1..2)")==std::string::npos,"No inline technical bounds on every question");
        check(out.find("Puissance adverse")!=std::string::npos,"Honest allocation visible before launch");
        auto simple_adaptive=form("1\n5\n2\n8\n1\n40\n2\n1\n");
        check(simple_adaptive.security.detector=="adaptive","Simple security menu selects adaptive detection");
        std::istringstream original(custom_session(1));std::vector<std::string> lines;std::string line;
        while(std::getline(original,line))lines.push_back(line);
        lines.at(5)="2";lines.at(23)="";
        // Adaptive-only fields now appear before the metric thresholds: use defaults for the remainder.
        lines.resize(24);std::ostringstream defaults;for(const auto& value:lines)defaults<<value<<'\n';defaults<<std::string(40,'\n');
        check(form(defaults.str()).security.detector=="adaptive","Custom defaults preserve the chosen adaptive security mode");
        terminal::Request disconnected;disconnected.nodes=6;auto partitioned=terminal::generate(disconnected);
        partitioned.links={{0,1,5,0},{1,2,5,0},{0,2,5,0},{3,4,5,0},{4,5,5,0},{3,5,5,0}};
        std::ostringstream review;terminal::summary(partitioned,review);
        check(review.str().find("reseau partitionne")!=std::string::npos && review.str().find("Topologie : anneau")==std::string::npos,"Disconnected cycles cannot be presented as one connected ring");
    }else if(name=="display"){
        terminal::Request r;r.height=8;r.attack="51";auto c=terminal::generate(r);
        const auto brief=new_output("brief"),detail=new_output("detail");std::ostringstream a,b;
        terminal::execute(c,brief,a,true,false);terminal::execute(c,detail,b,true,true);
        std::ifstream fa(brief+"/results.json"),fb(detail+"/results.json");std::stringstream sa,sb;sa<<fa.rdbuf();sb<<fb.rdbuf();
        check(sa.str()==sb.str(),"Display mode cannot change RNG, decisions or structured output");
        check(a.str().find(" node=")==std::string::npos && b.str().find(" received ")!=std::string::npos,"P2P receptions detailed only");
        check(a.str().size()<b.str().size(),"Default display materially reduces output");
    }else if(name=="window"){
        terminal::Request r;r.height=5;r.attack="51";r.adversary_percent=40;auto c=terminal::generate(r);
        sim::Simulator immediate(c);immediate.run();c.post_target_ticks=2000;
        sim::Simulator extended(c);extended.run();
        check(extended.target_tick()==immediate.target_tick(),"Window cannot alter pre-target trajectory");
        check(extended.production_end_tick()==extended.target_tick()+2000 && extended.metrics().blocks_produced>immediate.metrics().blocks_produced,"Normal production continues during finite observation window");
        check(immediate.cancelled_events()>0,"Obsolete timers cancelled without virtual wait");
        sim::Time last=0;for(const auto& e:immediate.trace())if(e.type!="alert")last=std::max(last,e.time);
        check(immediate.now()==last,"No final clock jump to inactive mining timer");
        for(const auto& a:immediate.detector().alerts())check(a.time<=last,"No silence induced by obsolete timers");
        c.attack.confirmations=10000;c.duration_ticks=extended.target_tick()+1;sim::Simulator censored(c);censored.run();
        check(censored.stop_reason()=="duration_ticks" && censored.outcome()=="inconclusive","Execution limit stays distinct from defense success");
        c.post_target_ticks=1000000001;reject([&]{sim::validate_config(c);});
    }else if(name=="four_profiles"){
        for(int type=1;type<=4;++type)for(bool defended:{false,true}){
            std::string transcript;auto c=form(simple_session(type,defended,true),&transcript);
            check(transcript.find("Entree invalide")!=std::string::npos,"Invalid attack-specific input rejected before corrected input");
            check(c.security.mode==(defended?"protected":"attacked"),"Defense choice transmitted");
            sim::Simulator s(c);s.run();check(s.metrics().blocks_produced>0 && !s.stop_reason().empty(),"Each simple terminal scenario finishes under bounds");
            if(type<=3)check(s.attack().retained>0,"Actual attack-specific withholding events");
            if(type==2)check(s.attack().publications>0 && (!defended || s.deferred_messages>0),"Selfish publications and real probation");
            if(type==3)check(s.pool().full_withheld>0 && (!defended || s.pool().shares_uncredited>0),"Pool retains full blocks and applies credit suspension");
            if(type==4)check(s.attack().filtered_messages>0 && (defended?s.reconnections>0:s.attack().success(s)),"Sybil filtering and operational reconnection");
            if(!defended)check(s.defenses().actions==0,"Unprotected runs have no hidden defense");
        }
    }else if(name=="four_custom"){
        for(int type=1;type<=4;++type){
            auto c=form(custom_session(type));
            check(c.seed==42 && c.nodes[0].hash_power==40 && c.nodes[1].hash_power==60,"Custom individual resources preserved");
            check(c.links.size()==4 && c.links[0].latency==7 && c.links[0].jitter==3,"Custom topology/latency/jitter transmitted");
            if(type==1)check(c.attack.confirmations==6,"Custom merchant threshold");
            if(type==3)check(c.attack.pool_members.size()==2 && c.security.min_shares==64,"Custom pool");
            if(type==4)check(c.attack.sybil_identities.size()==1 && !c.attack.capture_connections,"Custom identities and incomplete capture");
            sim::Simulator s(c);s.run();check(s.metrics().blocks_produced>0 && !s.stop_reason().empty(),"Each custom profile really executes");
        }
    }else if(name=="simple_app"){
        const auto previous=std::filesystem::current_path();const auto workspace=new_output("simple-app");
        std::filesystem::create_directories(workspace);std::filesystem::current_path(workspace);
        for(int type=1;type<=4;++type){
            std::ostringstream text;text<<"1\n5\n2\n8\n"<<type<<'\n';if(type!=4)text<<"40\n";
            text<<"1\n1\n3\n0\n";std::istringstream in(text.str());std::ostringstream out;
            check(terminal::interactive(in,out)==0,"Compare works from application without shell handoff");
            const auto printed=out.str();check(printed.find("SANS DEFENSE")!=std::string::npos && printed.find("AVEC DEFENSE")!=std::string::npos,"Both real comparisons displayed");
            for(const auto* hidden:{"seq=","node=","tick","late_p95","EVENEMENTS","Calibration honnete separee"})check(printed.find(hidden)==std::string::npos,"Simple mode never exposes internal operations");
            check(printed.find("Attaque reussie :")!=std::string::npos && printed.find("Defense efficace :")!=std::string::npos,"Understandable verdicts");
            const auto pair=std::filesystem::path("results/raw/interactive")/("run_"+std::to_string(type*2));
            auto a=sim::load_config((pair/"sans_defense/config.json").string());auto b=sim::load_config((pair/"avec_defense/config.json").string());
            check(!a.security.hardened_security && b.security.hardened_security,"Simple comparison switches hardened protections with defense");
            check(a.seed==b.seed,"Paired seed retained");a.security.mode=b.security.mode;a.security.hardened_security=b.security.hardened_security;
            check(sim::config_json(a)==sim::config_json(b),"Pair configurations differ only by defense mode");
            if(type==3)check(std::filesystem::exists(pair/"reference_honnete/results.json"),"Actual honest pool reference exported");
        }
        std::istringstream adaptive("1\n5\n2\n8\n1\n40\n2\n1\n3\n0\n");std::ostringstream adaptive_out;
        check(terminal::interactive(adaptive,adaptive_out)==0,"Adaptive comparison works inside application");
        auto paired=std::filesystem::path("results/raw/interactive/run_10");
        auto a=sim::load_config((paired/"sans_defense/config.json").string());auto b=sim::load_config((paired/"avec_defense/config.json").string());
        check(!a.security.hardened_security && b.security.hardened_security,"Adaptive comparison enables hardened policy only with defense");
        check(!a.security.reference_file.empty() && !b.security.reference_file.empty() && a.security.reference_file!=b.security.reference_file,"Each adaptive extractor uses its matching honest calibration");
        check(adaptive_out.str().find("Calibration honnete separee")==std::string::npos,"Simple adaptive calibration stays silent");
        std::filesystem::current_path(previous);
    }else if(name=="simple_verdict"){
        terminal::SimpleResult a,b;a.attack=b.attack="51";a.outcome="success";b.outcome="inconclusive";a.accepted=b.accepted=true;a.reversed=true;
        check(terminal::effectiveness(a,b)=="NON EVALUABLE","Inconclusive attack never becomes defense success");
        b.outcome="failed";check(terminal::effectiveness(a,b)=="OUI","Accepted protected payment can meet prevention criterion");
        b.accepted=false;check(terminal::effectiveness(a,b)=="NON EVALUABLE","Refusal is not proof of protection");
        a.attack=b.attack="selfish";a.power=b.power=.4;a.share=.6;b.share=.5;b.outcome="success";
        check(terminal::effectiveness(a,b)=="OUI","Attenuation can coexist with successful attack");
        std::ostringstream report;terminal::simple_report(b,report,"OUI");check(report.str().find("Attaque reussie : OUI")!=std::string::npos,"Attenuation never relabels attack as failed");
        a.attack=b.attack="withholding";a.retained=b.retained=10;a.pool_reward=b.pool_reward=100;a.payout=40;b.payout=20;
        check(terminal::effectiveness(a,b)=="OUI","Pool remuneration criterion");
        a.attack=b.attack="sybil";a.isolated=true;b.isolated=false;b.recovered=true;b.reconnections=1;b.outcome="failed";
        check(terminal::effectiveness(a,b)=="OUI","Recovery requires actual connection and adoption");b.recovered=false;
        check(terminal::effectiveness(a,b)=="NON","No recovery means no effective Sybil mitigation");
    }else throw std::invalid_argument("Unknown case");
    std::cout<<"PASS "<<name<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
