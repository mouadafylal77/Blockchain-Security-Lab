#pragma once
#include "sim/simulator.hpp"
#include <map>
#include <set>
#include <ostream>
namespace bsl::terminal {
// Read-only observer. Filtering affects stdout only, never the engine/exports.
class Display {
public:
    Display(const sim::Simulator& sim,std::ostream& out,bool detailed);
    void event(const sim::TraceRecord& event);
    void finish();
private:
    const sim::Simulator& sim_;
    std::ostream& out_;
    bool detailed_;
    std::set<std::string> shown_;
    std::map<std::string,std::uint64_t> alerts_;
    std::uint64_t hidden_=0;
};
}
