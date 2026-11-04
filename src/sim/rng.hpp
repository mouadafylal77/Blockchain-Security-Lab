#pragma once
#include "sim/event.hpp"
#include <random>
#include <cmath>
#include <stdexcept>
namespace bsl::sim {
class Rng {
public:
    explicit Rng(std::uint64_t seed) : engine_(seed) {}
    Time exponential_ticks(double mean) {
        if (!std::isfinite(mean) || mean <= 0) throw std::invalid_argument("Invalid exponential mean");
        // Open interval (0,1), using exactly 52 random bits, avoiding library distributions.
        const double u = (static_cast<double>(engine_() >> 12) + 0.5) / 4503599627370496.0;
        const double ticks = std::ceil(-std::log(u) * mean);
        // Configuration horizons are <= 1e12 ticks; larger waits are safely beyond them.
        if (!std::isfinite(ticks) || ticks > 1e15) return 1000000000000000ULL;
        return ticks < 1 ? 1 : static_cast<Time>(ticks);
    }
    Time uniform_ticks(Time maximum) {
        if (maximum == 0) return 0;
        const auto range = maximum + 1;
        if (range == 0) throw std::invalid_argument("Jitter range overflow");
        const auto threshold = -range % range;
        std::uint64_t value;
        do { value = engine_(); } while (value < threshold);
        return value % range;
    }
private:
    std::mt19937_64 engine_;
};
}
