#pragma once
#include "detection/detector.hpp"
namespace bsl::mitigation {
class Policy {
public:
    explicit Policy(detection::Config config) : config_(std::move(config)) {}
    void apply(const detection::Alert& alert);
    void review(const std::vector<detection::Feature>& features);
    bool payment_allowed(std::size_t node, detection::Time now) const;
    unsigned confirmations(std::size_t node, unsigned base, detection::Time now) const;
    detection::Time delivery_delay(std::size_t node,std::size_t peer,detection::Time now) const;
    bool credit_share(std::size_t member,detection::Time now) const;
    detection::Time hold_until(std::size_t node) const { return risk_until_.count(node)?risk_until_.at(node):0; }
    detection::Time strengthened_until(std::size_t node) const { return confirmation_until_.count(node)?confirmation_until_.at(node):0; }
    std::uint64_t actions = 0, payment_checks = 0, payment_deferrals = 0;
private:
    detection::Config config_;
    std::map<std::size_t,detection::Time> risk_until_, confirmation_until_;
    std::map<std::size_t,detection::Time> episode_start_;
    std::map<std::size_t,unsigned> extra_confirmations_;
    std::map<std::pair<std::size_t,std::size_t>,detection::Time> probation_until_;
    std::map<std::pair<std::size_t,std::size_t>,std::uint64_t> publication_samples_;
    std::map<std::size_t,detection::Time> audit_until_;
    std::map<std::size_t,std::uint64_t> pool_samples_;
};
}
