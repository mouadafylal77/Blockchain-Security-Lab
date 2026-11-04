#pragma once
#include "sim/simulator.hpp"
namespace bsl::metrics {
std::string results_json(const sim::Simulator& simulator);
void write_results(const sim::Simulator& simulator, const std::string& output_directory);
}
