#include "terminal/display.hpp"
namespace bsl::terminal {
Display::Display(const sim::Simulator& sim,std::ostream& out,bool detailed):sim_(sim),out_(out),detailed_(detailed){
    out_<<"\n----------- EVENEMENTS "<<(detailed?"DETAILLES":"SYNTHETIQUES")<<" -----------\n"
        <<"Vue pedagogique : les informations privees ne sont pas des entrees de detection.\n";
    if(!detailed)out_<<"Les receptions P2P et repetitions sont conservees dans results.json.\n";
}
void Display::event(const sim::TraceRecord& e){
    const auto metric=e.detail.substr(0,e.detail.find('='));
    if(e.type=="alert")++alerts_[metric];
    if(detailed_){
        out_<<"t="<<e.time<<" seq="<<e.event_sequence<<" node="<<sim_.nodes()[e.node].id<<' '<<e.type;
        if(!e.hash.empty())out_<<" block="<<e.hash;
        if(e.type=="discovery" || e.type=="withheld")out_<<" parent="<<sim_.blocks().at(e.hash).block.previous_hash;
        if(e.depth)out_<<" depth/count="<<e.depth;
        if(!e.detail.empty())out_<<' '<<e.detail;
        out_<<'\n';return;
    }
    std::string text,key;
    const bool observer=e.node==sim_.config().observer;
    if(e.type=="discovery")text="Bloc decouvert par "+sim_.nodes()[e.node].id+" (hauteur "+std::to_string(sim_.blocks().at(e.hash).block.index)+")";
    else if(e.type=="fork"){key="fork/"+e.hash;text="Fork observe : une branche concurrente";}
    else if(e.type=="reorganization"){key="reorg/"+e.new_tip+(observer?"/observer":"");text="Reorganisation locale : "+std::to_string(e.depth)+" bloc(s) retires";}
    else if(e.type=="publication"){key="publication/"+std::to_string(e.time);text="Publication d'une branche privee";}
    else if(e.type=="withheld"){key="withheld";text="Retention de blocs [PRIVE, vue pedagogique] ; detail dans l'export";}
    else if(e.type=="alert"){key="alert/"+metric+(observer?"/observer":"");text="Alerte de comportement (pas une preuve) : "+e.detail;}
    else if(e.type=="security_action"){key="action/"+e.detail+(observer?"/observer":"");text="Defense activee : "+e.detail;}
    else if(e.type=="peer_reconnection"){key="peer/"+std::to_string(e.node);text="Reconnexion et rattrapage P2P";}
    else if(e.type=="payment_accepted")text="Paiement accepte : "+std::to_string(e.depth)+" confirmations, "+e.detail;
    else if(e.type=="payment_reversed")text="Paiement accepte annule par une reorganisation";
    else if(e.type=="payment_restored")text="Paiement de nouveau present dans la chaine locale";
    else if(e.type=="target_reached")text="Hauteur cible atteinte ; observation de fin selon la configuration";
    else if(e.type=="production_closed")text="Fin de production : "+e.detail+" ; livraison des messages restants";
    else{++hidden_;return;}
    if(!key.empty() && !shown_.insert(key).second){++hidden_;return;}
    out_<<"[tick "<<e.time<<"] "<<text<<" - "<<sim_.nodes()[e.node].id<<'\n';
}
void Display::finish(){
    if(!detailed_)out_<<"Affichage : "<<hidden_<<" receptions/repetitions non affichees ; export integral conserve.\n";
    if(!alerts_.empty()){
        out_<<"Alertes completes par metrique :";
        for(const auto& a:alerts_)out_<<' '<<a.first<<'='<<a.second;
        out_<<"\nLes totaux incluent les observations locales repetees ; ils ne comptent pas des attaques distinctes.\n";
    }
}
}
