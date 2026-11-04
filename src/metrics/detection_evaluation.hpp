#pragma once
#include "detection/detector.hpp"
namespace bsl::metrics {
struct Confusion {
    unsigned tp=0, fp=0, tn=0, fn=0;
    double precision() const { return tp+fp ? double(tp)/(tp+fp) : 0; }
    double recall() const { return tp+fn ? double(tp)/(tp+fn) : 0; }
    double f1() const { const double p=precision(), r=recall(); return p+r ? 2*p*r/(p+r) : 0; }
};
// One unit = (run, behavior family). Labels supplied only after simulation.
Confusion evaluate(const std::vector<detection::Alert>& alerts, const std::string& exposed_family);
}
