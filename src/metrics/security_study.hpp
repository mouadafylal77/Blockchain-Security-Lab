#pragma once
#include <cstdint>
#include <cstddef>
#include <string>
namespace bsl::sim {class Simulator;}
namespace bsl::metrics {
struct MerchantDecision {
    std::uint64_t time=0,sequence=0,confirmations=0,required=0,hold_until=0,strengthened_until=0;
    bool payment_present=false,base_ready=false,hold_active=false,eligible=false;
    std::string tip;
};
struct DeferredDelivery {
    std::uint64_t time,sequence,delay;
    std::size_t receiver,sender,producer;
    std::string hash;
};
std::string study_json(const sim::Simulator& simulator);
}
