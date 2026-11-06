#include "mitigation/policy.hpp"
#include <algorithm>
#include <cmath>
namespace bsl::mitigation {
void Policy::apply(const detection::Alert& a) {
    if(config_.mode!="protected")return;
    if(config_.hardened_security && a.severity=="informative")return;
    if(config_.hardened_security && a.metric=="share_deficit" && !a.evidence_sufficient)return;
    if(config_.hardened_security && a.metric=="late_p95" && (!a.evidence_sufficient || a.component.rfind("producer:",0)!=0))return;
    if(config_.hardened_security && a.metric=="share_deficit" && audit_until_[a.node]>a.time)return;
    if(a.metric=="share_deficit" && config_.mitigate_pool){audit_until_[a.node]=a.time+config_.hold_ticks;++actions;}
    const std::string prefix=config_.hardened_security?"producer:":"peer:";
    if(a.metric=="late_p95" && config_.mitigate_publication && a.component.rfind(prefix,0)==0){
        const auto peer=static_cast<std::size_t>(std::stoull(a.component.substr(prefix.size())));
        if(config_.hardened_security && probation_until_[{a.node,peer}]>a.time)return;
        if(config_.hardened_security && a.samples && a.samples<=publication_samples_[{a.node,peer}])return;
        publication_samples_[{a.node,peer}]=a.samples;
        probation_until_[{a.node,peer}]=a.time+config_.hold_ticks;
        ++actions;
    }
    if((a.behavior=="chain_instability" || (config_.proactive_signals && (a.metric=="fork_rate" || a.metric=="silence_ticks"))) && config_.mitigate_confirmations){
        const bool active=confirmation_until_.count(a.node) && a.time<confirmation_until_.at(a.node);
        if(!active)episode_start_[a.node]=a.time;
        const auto extra=static_cast<unsigned>(std::min(10000.0,(a.metric=="reorg_depth"?std::ceil(a.value):0.0)))+config_.confirmation_increment;
        if(config_.hardened_security && active){
            const auto elevated=std::min(config_.max_confirmations,extra);
            if(elevated>extra_confirmations_[a.node]){extra_confirmations_[a.node]=elevated;++actions;}
            return; // Escalation does not extend either deadline.
        }
        risk_until_[a.node]=a.time+config_.hold_ticks;
        if(config_.max_payment_hold_ticks)risk_until_[a.node]=std::min(risk_until_[a.node],episode_start_.at(a.node)+config_.max_payment_hold_ticks);
        else if(config_.hardened_security)risk_until_[a.node]=std::min(risk_until_[a.node],episode_start_.at(a.node)+config_.confirmation_ticks);
        confirmation_until_[a.node]=a.time+config_.confirmation_ticks;
        extra_confirmations_[a.node]=active?std::max(extra_confirmations_[a.node],std::min(config_.max_confirmations,extra)):std::min(config_.max_confirmations,extra);
        ++actions;
    }
}
void Policy::review(const std::vector<detection::Feature>& features){
    if(!config_.hardened_security || config_.mode!="protected")return;
    for(const auto& f:features)if(f.metric=="share_deficit"){
        const bool fresh=f.samples>pool_samples_[f.node];pool_samples_[f.node]=f.samples;
        if(!f.evidence_sufficient)continue;
        if(f.value<=config_.thresholds.at("share_deficit"))audit_until_[f.node]=f.time;
        else if(fresh && audit_until_[f.node]>f.time)audit_until_[f.node]=f.time+config_.hold_ticks;
    }
}
bool Policy::payment_allowed(std::size_t node,detection::Time now) const {
    return !risk_until_.count(node)||now>=risk_until_.at(node);
}
unsigned Policy::confirmations(std::size_t node,unsigned base,detection::Time now) const {
    if(config_.hardened_security && config_.mode=="protected" && config_.mitigate_confirmations)base=std::max(base,config_.preventive_confirmations);
    if(!extra_confirmations_.count(node)||now>=confirmation_until_.at(node))return base;
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
