#include "detection/detector.hpp"
#include "detection/feature_extractor.hpp"
#include "metrics/detection_evaluation.hpp"
#include <iostream>
#include <stdexcept>
#include <cmath>
using namespace bsl;
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
int main(){try{
    detection::FeatureExtractor w(10);
    w.record(1,0,"age",2); w.record(5,0,"age",4); w.record(11,0,"age",6);
    check(w.values(11,0,"age")==std::vector<double>({4,6}),"Window must be (t-W,t]");
    detection::Config c;c.mode="attacked";c.thresholds["reorg_depth"]=2;
    detection::Detector d(c);
    check(d.observe({{1,0,"reorg_depth",2,"node"}}).empty(),"Equality is not an exceedance");
    auto alerts=d.observe({{2,0,"reorg_depth",3,"node"}});
    check(alerts.size()==1 && alerts[0].threshold==2,"Fixed threshold");
    auto cf=metrics::evaluate(alerts,"chain_instability");check(cf.tp==1 && cf.tn==3 && cf.f1()==1,"Confusion TP");
    cf=metrics::evaluate(alerts,"");check(cf.fp==1 && cf.tn==3,"Confusion FP");
    cf=metrics::evaluate({},"pool_deficit");check(cf.fn==1 && cf.recall()==0,"Confusion FN");
    bool rejected=false;try{c.audit_ticks=0;detection::validate(c);}catch(const std::invalid_argument&){rejected=true;}
    check(rejected,"Bad parameters rejected");
    c=detection::Config{};c.mode="attacked";c.detector="adaptive";
    detection::Detector adaptive(c,{{"reorg_depth",{10,1,4}}});
    check(adaptive.observe({{1,0,"reorg_depth",2,"node"}}).empty(),"Past calibrated variance accepts normal change");
    const auto a=adaptive.observe({{2,0,"reorg_depth",8,"node"}});
    check(a.size()==1 && std::abs(a[0].threshold-(1.1+3*std::sqrt(3.69)))<1e-10,"EWMA threshold uses only prior samples");
    detection::Detector empty(c);
    check(empty.observe({{1,0,"reorg_depth",100,"node"}}).empty() && empty.insufficient_samples()==1,"Missing calibration must not invent a threshold");
    detection::Detector zero(c,{{"reorg_depth",{10,0,0}}});
    check(zero.observe({{1,0,"reorg_depth",0.6,"node"}}).size()==1,"Zero dispersion has a positive floor");
    detection::Detector poisoned(c,{{"reorg_depth",{10,0,0}}});
    for(unsigned i=1;i<200;++i) poisoned.observe({{i,0,"reorg_depth",0.002*i,"node"}});
    check(poisoned.observe({{1000,0,"reorg_depth",5,"node"}}).size()==1,"Slow contamination cannot raise thresholds without bound");
    c.mode="calibration";
    detection::Detector calibration(c);
    for(unsigned i=1;i<=6;++i) calibration.observe({{i,0,"reorg_depth",double(i),"node"}});
    const auto r=calibration.calibration().at("reorg_depth");
    check(r.count==6 && r.mean==3.5 && std::abs(r.variance-35.0/12)<1e-10,"Honest reference uses actual Welford samples");
    check(calibration.alerts().empty(),"Adaptive baseline is calibration, not retrospective detection");
    rejected=false;try{detection::Detector invalid(c,{{"reorg_depth",{1,0,0}}});}catch(const std::invalid_argument&){rejected=true;}
    check(rejected,"Too few calibration samples rejected");
    rejected=false;try{detection::Detector invalid(c,{{"reorg_depth",{10,0,-1}}});}catch(const std::invalid_argument&){rejected=true;}
    check(rejected,"Negative calibration dispersion rejected");
    std::cout<<"PASS detection\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
