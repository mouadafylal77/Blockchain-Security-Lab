#pragma once
#include "sim/config.hpp"
#include <ostream>
namespace bsl::sim {class Simulator;}
namespace bsl::terminal {
struct SimpleResult {
    std::string attack,outcome,reference_file;
    double power=0,share=0,payout=0,pool_reward=0;
    bool accepted=false,reversed=false,isolated=false,recovered=false,hard_limit=false;
    unsigned confirmations=0,required=0;
    std::uint64_t retained=0,reconnections=0;
};
SimpleResult simple_result(const sim::Simulator& sim);
std::string effectiveness(const SimpleResult& without,const SimpleResult& with);
void simple_summary(const sim::Config& config,std::ostream& out);
void simple_report(const SimpleResult& result,std::ostream& out,const std::string& defense="NON EVALUABLE",double honest_pool_reward=-1);
void compare_simple(sim::Config config,const std::string& output_root,std::ostream& out,bool hardened=false);
}
