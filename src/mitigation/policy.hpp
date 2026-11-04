#pragma once
#include "detection/detector.hpp"
namespace bsl::mitigation {
class Policy {
public:
    explicit Policy(detection::Config config) : config_(std::move(config)) {}
    void apply(const detection::Alert& alert);
    bool payment_allowed(std::size_t node, detection::Time now) const;
    unsigned confirmations(std::size_t node, unsigned base, detection::Time now) const;
    detection::Time delivery_delay(std::size_t node,std::size_t peer,detection::Time now) const;
    bool credit_share(std::size_t member,detection::Time now) const;
    std::uint64_t actions = 0, payment_checks = 0, payment_deferrals = 0;
private:
    detection::Config config_;
    std::map<std::size_t,detection::Time> risk_until_;
    std::map<std::size_t,unsigned> extra_confirmations_;
    std::map<std::pair<std::size_t,std::size_t>,detection::Time> probation_until_;
    std::map<std::size_t,detection::Time> audit_until_;
};
}
