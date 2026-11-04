#include "sim/simulator.hpp"
#include "metrics/json_exporter.hpp"
#include "metrics/detection_evaluation.hpp"
#include <iostream>
#include <stdexcept>
using namespace bsl;
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
double share(const sim::Simulator& s){
    const auto active=s.nodes().at(s.config().attack.target).chain.active_chain();unsigned own=0;
    for(std::size_t i=1;i<active.size();++i)if(s.blocks().at(active[i]).miner==s.config().attack.adversary)++own;
    return active.size()>1?double(own)/(active.size()-1):0;
}
int main(){try{
    unsigned false_positives=0;
    for(const std::string name:{"attack_51","selfish_mining","block_withholding","sybil"}){
        const auto source=std::string(BSL_SOURCE_DIR)+"/configs/m4/";
        sim::Simulator calibration(sim::load_config(source+name+"_calibration.json"));calibration.run();
        const auto output=std::string(BSL_BINARY_DIR)+"/security-reference/"+name;
        metrics::write_results(calibration,output);
        for(const std::string detector:{"fixed","adaptive"}){
            auto config=sim::load_config(source+name+"_"+detector+"_baseline.json");
            if(detector=="adaptive")config.security.reference_file=output+"/calibration.json";
            sim::Simulator baseline(config);baseline.run();
            check(baseline.defenses().actions==0&&baseline.deferred_messages==0&&baseline.reconnections==0,"Baseline alerts must not activate hidden defenses");
            false_positives+=metrics::evaluate(baseline.detector().alerts(),"").fp;
            config=sim::load_config(source+name+"_"+detector+"_attacked.json");
            if(detector=="adaptive")config.security.reference_file=output+"/calibration.json";
            sim::Simulator attacked(config);attacked.run();
            std::string family=name=="attack_51"?"chain_instability":name=="selfish_mining"?"publication_anomaly":name=="block_withholding"?"pool_deficit":"connectivity_anomaly";
            check(metrics::evaluate(attacked.detector().alerts(),family).tp==1,"Controlled attack example must be observed by both detectors");
            config=sim::load_config(source+name+"_"+detector+"_protected.json");
            if(detector=="adaptive")config.security.reference_file=output+"/calibration.json";
            sim::Simulator protected_run(config);protected_run.run();
            if(name=="attack_51")check(protected_run.attack().success(protected_run),"Late alarms do not undo an already accepted double spend");
            if(name=="selfish_mining")check(protected_run.deferred_messages>0&&share(protected_run)<share(attacked),"Experimental probation must reduce the measured advantage in the selected example");
            if(name=="block_withholding")check(protected_run.pool().shares_uncredited>0 && protected_run.pool().shares[0]<attacked.pool().shares[0],"Audit must change actual payout credits");
            if(name=="sybil")check(protected_run.reconnections>0&&!protected_run.attack().success(protected_run),"Bootstrap reconnection must end complete isolation");
            sim::Simulator repeat(config);repeat.run();
            check(metrics::results_json(repeat)==metrics::results_json(protected_run),"Protected examples must reproduce byte-for-byte");
        }
    }
    check(false_positives>0,"Noisy honest runs must retain their measured false positives");
    std::cout<<"PASS controlled M4 examples\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
