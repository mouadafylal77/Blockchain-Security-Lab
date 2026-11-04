#include "mitigation/policy.hpp"
#include <algorithm>
#include <cmath>
namespace bsl::mitigation {
void Policy::apply(const detection::Alert& a) {
    if(config_.mode!="protected")return;
    if(a.metric=="share_deficit" && config_.mitigate_pool){audit_until_[a.node]=a.time+config_.hold_ticks;++actions;}
    if(a.metric=="late_p95" && config_.mitigate_publication && a.component.rfind("peer:",0)==0){
        const auto peer=static_cast<std::size_t>(std::stoull(a.component.substr(5)));
        probation_until_[{a.node,peer}]=a.time+config_.hold_ticks;
        ++actions;
    }
    if(a.behavior=="chain_instability" && config_.mitigate_confirmations){
        risk_until_[a.node]=a.time+config_.hold_ticks;
        const auto extra=static_cast<unsigned>(std::min(10000.0,std::ceil(a.value)))+config_.confirmation_increment;
        extra_confirmations_[a.node]=std::min(config_.max_confirmations,extra);
        ++actions;
    }
}
bool Policy::payment_allowed(std::size_t node,detection::Time now) const {
    return !risk_until_.count(node)||now>=risk_until_.at(node);
}
unsigned Policy::confirmations(std::size_t node,unsigned base,detection::Time now) const {
    if(!extra_confirmations_.count(node)||payment_allowed(node,now))return base;
    return std::max(base,std::min(config_.max_confirmations,base+extra_confirmations_.at(node)));
}
detection::Time Policy::delivery_delay(std::size_t node,std::size_t peer,detection::Time now) const{
    const auto it=probation_until_.find({node,peer});
    return it!=probation_until_.end()&&now<it->second?config_.publication_delay:0;
}
bool Policy::credit_share(std::size_t member,detection::Time now) const{
    return !audit_until_.count(member)||now>=audit_until_.at(member);
}
}
