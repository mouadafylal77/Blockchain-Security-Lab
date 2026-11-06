#pragma once
#include "sim/simulator.hpp"
namespace bsl::metrics {
std::string results_json(const sim::Simulator& simulator);
void validate_output_directory(const std::string& directory);
void write_results(const sim::Simulator& simulator, const std::string& output_directory);
}
