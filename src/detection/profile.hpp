#pragma once
#include <string>
namespace bsl::sim {struct Config;}
namespace bsl::detection {
std::string network_profile(const sim::Config& config);
void validate_reference_profile(const sim::Config& config);
}
