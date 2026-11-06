#include "metrics/security_study.hpp"
#include "sim/simulator.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <numeric>
#include <set>
namespace bsl::metrics {
std::string study_json(const sim::Simulator& sim){
    using Json=nlohmann::json;
    Json merchant={{"applicable",sim.config().attack.type=="51"}};
    if(sim.config().attack.type=="51"){
        Json records=Json::array(),first=nullptr,first_present=nullptr,first_action=nullptr,acceptance_event=nullptr;
        unsigned actions_before_acceptance=0,actions_while_unaccepted=0;bool accepted_in_trace=false;
        for(const auto& d:sim.merchant_decisions()){
            records.push_back({{"time",d.time},{"sequence",d.sequence},{"tip",d.tip},{"payment_present",d.payment_present},
                {"confirmations",d.confirmations},{"required",d.required},{"base_ready",d.base_ready},
                {"hold_active",d.hold_active},{"hold_until",d.hold_until},{"strengthened_until",d.strengthened_until},{"eligible",d.eligible}});
            if(d.payment_present && first_present.is_null())first_present=d.time;
            if(d.base_ready && first.is_null())first=d.time;
        }
        const auto accepted=sim.attack().accepted;
        std::uint64_t reversals=0;
        for(std::size_t index=0;index<sim.trace().size();++index){const auto& e=sim.trace()[index];
            if(e.node!=sim.config().attack.target)continue;
            if(e.type=="payment_reversed")++reversals;
            if(e.type=="payment_accepted"){
                accepted_in_trace=true;acceptance_event={{"time",e.time},{"event_sequence",e.event_sequence},{"trace_index",index}};
            }
            if(e.type!="security_action")continue;
            if(first_action.is_null())first_action=e.time;
            if(!accepted_in_trace)++actions_while_unaccepted;
            if(!accepted || e.time<sim.attack().accepted_at)++actions_before_acceptance;
        }
        merchant.update({{"decisions",records},{"accepted",accepted},{"non_acceptance_is_censored",!accepted},
            {"reversal_events",reversals},{"ever_reversed",reversals>0},
            {"first_payment_observed_at",first_present},{"first_base_ready_at",first},{"first_action_at",first_action},
            {"actions_strictly_before_acceptance",accepted?Json(actions_before_acceptance):Json(nullptr)},
            {"actions_causally_before_acceptance",accepted?Json(actions_while_unaccepted):Json(nullptr)},
            {"actions_while_unaccepted",actions_while_unaccepted},{"acceptance_event",acceptance_event},
            {"accepted_at",accepted?Json(sim.attack().accepted_at):Json(nullptr)},
            {"opportunity_delay_ticks",accepted && !first.is_null()?Json(sim.attack().accepted_at-first.get<std::uint64_t>()):Json(nullptr)},
            {"double_spend",sim.attack().success(sim)},
            {"warning","Base-ready is an opportunity on the realized protected trajectory, not a no-defense counterfactual; same-tick causal order is in decisions/trace."}});
    }
    // Ground-truth roles below are post-run evaluation only. Never supplied to a detector/policy.
    const auto adverse=[&](std::size_t node){
        const auto& a=sim.config().attack;
        return a.enabled && a.type!="none" && (sim.attack().controls(node)
            || (a.type=="sybil" && std::find(a.sybil_identities.begin(),a.sybil_identities.end(),node)!=a.sybil_identities.end()));
    };
    std::uint64_t honest_sender=0,honest_producer=0,honest_added=0;std::set<std::string> honest_payloads;
    for(const auto& d:sim.deferred_deliveries()){
        if(!adverse(d.sender))++honest_sender;
        if(!adverse(d.producer)){++honest_producer;honest_added+=d.delay;honest_payloads.insert(d.hash);}
    }
    Json probation={{"messages",sim.deferred_messages},{"honest_sender_messages",honest_sender},
        {"adverse_sender_messages",sim.deferred_messages-honest_sender},{"honest_producer_messages",honest_producer},
        {"adverse_producer_messages",sim.deferred_messages-honest_producer},{"honest_unique_payloads",honest_payloads.size()},
        {"honest_producer_added_ticks",honest_added},{"all_added_ticks",sim.deferred_ticks},
        {"warning","Sender and producer categories overlap. Counts are message events, not unique economic losses; ticks are not CPU time."}};
    Json members=Json::array();std::uint64_t honest_uncredited=0,adverse_uncredited=0,honest_held=0,adverse_held=0,honest_released=0,honest_credit_ticks=0;
    const auto& p=sim.pool();const auto credited=std::accumulate(p.shares.begin(),p.shares.end(),std::uint64_t{0});
    const auto reward=attacks::pool_reward(sim);double honest_payout=0,adverse_payout=0;
    for(std::size_t i=0;i<sim.nodes().size();++i)if(p.member(i)){
        const auto uncredited=p.observed_shares[i]-p.shares[i];
        const double payout=credited?reward*p.shares[i]/credited:0;
        if(adverse(i)){adverse_uncredited+=uncredited;adverse_payout+=payout;}else{honest_uncredited+=uncredited;honest_payout+=payout;}
        if(adverse(i))adverse_held+=p.held_ever[i];else{honest_held+=p.held_ever[i];honest_released+=p.released[i];honest_credit_ticks+=p.credit_ticks[i];}
        members.push_back({{"node",sim.nodes()[i].id},{"adverse_evaluation_only",adverse(i)},{"observed",p.observed_shares[i]},
            {"credited",p.shares[i]},{"uncredited",uncredited},{"submitted_full",p.submitted_full[i]},{"payout_units",payout}});
        members.back().update({{"escrow_current",p.escrowed[i]},{"escrow_ever",p.held_ever[i]},
            {"escrow_released",p.released[i]},{"escrow_credit_ticks",p.credit_ticks[i]}});
    }
    Json pool={{"applicable",!sim.config().attack.pool_members.empty()},{"members",members},{"honest_uncredited",honest_uncredited},
        {"honest_escrow_ever",honest_held},{"adverse_escrow_ever",adverse_held},{"honest_escrow_released",honest_released},{"honest_escrow_credit_ticks",honest_credit_ticks},
        {"adverse_uncredited",adverse_uncredited},{"honest_payout_units",honest_payout},{"adverse_payout_units",adverse_payout},
        {"pool_reward_units",reward},{"warning","Pool loss requires a paired honest reference; withheld solutions are not an exact counterfactual loss."}};
    Json peers=Json::array();
    const auto initial_network=attacks::network_config(sim.config());
    for(std::size_t i=0;i<sim.nodes().size();++i){
        Json request=nullptr,connection=nullptr,adoption=nullptr,height=nullptr;std::uint64_t requests=0;
        Json first_block=nullptr;
        bool initially_isolated=false;Json independent_connection=nullptr;
        if(sim.config().attack.enabled && sim.config().attack.type=="sybil" && i==sim.config().attack.target){
            unsigned initial_peers=0,independent=0;
            for(const auto& link:initial_network.links)if(link.from==i || link.to==i){
                const auto peer=link.from==i?link.to:link.from;++initial_peers;if(!adverse(peer))++independent;
            }
            initially_isolated=initial_peers>0 && independent==0;
        }
        for(const auto& e:sim.trace())if(e.node==i){
            if(first_block.is_null() && (e.type=="tip_extension" || e.type=="reorganization"))first_block=e.time;
            if(e.type=="bootstrap_request"){++requests;if(request.is_null()){request=e.time;height=e.depth;}}
            const bool connected=e.type=="peer_reconnection" || e.type=="peer_connection_inbound";
            if(connected && connection.is_null())connection=e.time;
            if(connected && independent_connection.is_null() && !adverse(static_cast<std::size_t>(std::stoull(e.old_tip))))independent_connection=e.time;
            if(!connection.is_null() && adoption.is_null() && (e.type=="tip_extension" || e.type=="reorganization"))adoption=e.time;
        }
        peers.push_back({{"node",sim.nodes()[i].id},{"requests",requests},{"height_at_first_request",height},
            {"initial_no_block_ticks",first_block.is_null()?Json(sim.now()):first_block},{"initial_no_block_censored",first_block.is_null()},
            {"initial_exclusive_adversarial_connectivity",initially_isolated},
            {"initial_eclipse_ticks",initially_isolated?(independent_connection.is_null()?Json(sim.now()):independent_connection):Json(0)},
            {"initial_eclipse_censored",initially_isolated && independent_connection.is_null()},
            {"first_request_at",request},{"first_connection_at",connection},{"first_adoption_at",adoption},
            {"recovery_from_request_ticks",!request.is_null() && !adoption.is_null()?Json(adoption.get<std::uint64_t>()-request.get<std::uint64_t>()):Json(nullptr)},
            {"warning","First adoption after connection is not full convergence or proof of exclusive synchronization provenance. Null means not observed."}});
    }
    return Json{{"merchant",merchant},{"probation",probation},{"pool",pool},{"sybil",peers},
        {"accounting",{{"merchant_decision_records",sim.merchant_decisions().size()},{"deferred_delivery_records",sim.deferred_deliveries().size()},
            {"reference_cells",sim.detector().reference().size()},{"warning","Logical records, not measured RSS or detector CPU time."}}}}.dump();
}
}
