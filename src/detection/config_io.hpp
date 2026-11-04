#pragma once
#include "detection/detector.hpp"
namespace bsl::detection {
Config parse_config(const std::string& text);
std::string config_json(const Config& config);
std::map<std::string, Reference> load_reference(const Config& config);
std::string reference_json(const Config& config, const Detector& detector);
}
