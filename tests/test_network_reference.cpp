#include "detection/profile.hpp"
#include "detection/config_io.hpp"
#include "terminal/interactive.hpp"
#include "metrics/json_exporter.hpp"
#include <iostream>
#include <stdexcept>
using namespace bsl;
void check(bool v,const char* m){if(!v)throw std::runtime_error(m);}
template<class F>void rejects(F f){bool failed=false;try{f();}catch(const std::exception&){failed=true;}check(failed,"Mismatched reference must fail");}
int main(){try{
    detection::Config training;training.mode="calibration";training.stratify_reference=true;
    detection::Detector cal(training);
    for(unsigned t=1;t<=6;++t)cal.observe({{t,0,"late_p95",10,"peer:1"},{t,1,"late_p95",100,"peer:0"}});
    check(cal.calibration().at("0/peer:1/late_p95").count==6,"Calibration maintains separate honest strata");
    auto settings=training;settings.mode="attacked";settings.detector="adaptive";
    detection::Detector stratified(settings,cal.calibration());
    auto alerts=stratified.observe({{7,0,"late_p95",60,"peer:1"},{7,1,"late_p95",100,"peer:0"}});
    check(alerts.size()==1 && alerts[0].node==0,"Honest slow peer does not mask anomaly on normally fast peer");
    stratified.observe({{7,2,"late_p95",200,"peer:0"}});
    check(stratified.insufficient_samples()==1,"Uncalibrated strata are explicit, never invented");
    auto fixed=settings;fixed.detector="fixed";detection::Detector old(fixed);
    check(old.observe({{7,1,"late_p95",100,"peer:0"}}).size()==1,"Fixed threshold baseline retained");
    terminal::Request r;r.nodes=30;r.miners=10;r.height=15;r.attack="51";r.adversary_percent=40;
    auto c=terminal::generate(r);c.attack.enabled=false;c.target_height=0;c.max_blocks=80;c.duration_ticks=20000;
    c.seed=41001;c.security.mode="calibration";c.security.stratify_reference=true;c.security.strict_reference=true;
    c.security.detector="adaptive";c.security.audit_ticks=100;
    sim::Simulator reference(c);reference.run();
    const auto directory=std::string(BSL_BINARY_DIR)+"/security-refinement/network30";
    metrics::write_results(reference,directory);
    const auto profile=detection::network_profile(c);c.seed=42001;check(detection::network_profile(c)==profile,"Seed is not network profile");
    c.security.mode="baseline";c.security.reference_file=directory+"/calibration.json";
    sim::Simulator validation(c);validation.run();metrics::write_results(validation,directory+"/validation");
    auto other=c;other.links[0].latency+=1;rejects([&]{sim::Simulator mismatch(other);});
    other=c;other.mean_block_ticks+=1;rejects([&]{sim::Simulator mismatch(other);});
    other=c;other.security.stratify_reference=false;rejects([&]{sim::Simulator mismatch(other);});
    auto f=c;f.security.detector="fixed";sim::Simulator baseline(f);baseline.run();metrics::write_results(baseline,directory+"/fixed");
    auto protected_config=c;protected_config.security.mode="protected";
    sim::Simulator adaptive_protected(protected_config);adaptive_protected.run();metrics::write_results(adaptive_protected,directory+"/honest_protected");
    protected_config.security.detector="fixed";
    sim::Simulator fixed_protected(protected_config);fixed_protected.run();metrics::write_results(fixed_protected,directory+"/fixed_honest_protected");
    std::cout<<"Independent honest validation: fixed="<<baseline.detector().alerts().size()<<" stratified="<<validation.detector().alerts().size()
        <<" missing="<<validation.detector().insufficient_samples()<<'\n';
    std::cout<<"PASS context-bound honest network reference\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
