#pragma once
#include "sim/config.hpp"
#include <istream>
#include <ostream>
#include <string>
#include <optional>
namespace bsl::terminal {
struct SimpleResult;
struct Request {
    std::size_t nodes=4, miners=2;
    std::uint64_t height=20, seed=42;
    std::string attack="none";
    bool defenses=false;
    std::optional<double> adversary_percent;
    std::size_t pool_honest_miners=0, sybil_count=0; // 0 selects all eligible participants.
    std::string detector="fixed";
};
sim::Config generate(const Request& request);
sim::Config configure(std::istream& input,std::ostream& output,bool* detailed=nullptr,bool* simple=nullptr);
void summary(const sim::Config& config,std::ostream& output);
void execute(sim::Config config,const std::string& directory,std::ostream& output,bool pedagogical=true,bool detailed=false,SimpleResult* result=nullptr,bool quiet=false);
int interactive(std::istream& input,std::ostream& output);
}
