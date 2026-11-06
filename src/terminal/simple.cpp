#include "terminal/simple.hpp"
#include "terminal/interactive.hpp"
#include "sim/simulator.hpp"
#include <filesystem>
#include <numeric>
#include <iomanip>
namespace bsl::terminal {
namespace {
std::string name(const std::string& type){
    if(type=="51")return "Double depense (51 %)";
    if(type=="selfish")return "Selfish Mining";
    if(type=="withholding")return "Block Withholding";
    if(type=="sybil")return "Sybil";
    return "Aucune";
}
const char* yes(bool value){return value?"OUI":"NON";}
}
SimpleResult simple_result(const sim::Simulator& sim){
    SimpleResult r;const auto& c=sim.config();r.attack=c.attack.type;r.outcome=sim.outcome();r.reference_file=c.security.reference_file;
    r.accepted=sim.attack().accepted;r.reversed=sim.attack().success(sim) && r.attack=="51";
    r.confirmations=sim.attack().accepted_confirmations;r.required=sim.attack().accepted_required;
    r.retained=sim.attack().retained;r.share=0;r.pool_reward=attacks::pool_reward(sim);
    double total=0,adverse=0;for(std::size_t i=0;i<c.nodes.size();++i){total+=c.nodes[i].hash_power;if(sim.attack().controls(i))adverse+=c.nodes[i].hash_power;}
    r.power=total?adverse/total:0;
    if(r.attack!="none"){
        const auto active=sim.nodes()[c.attack.target].chain.active_chain();unsigned own=0;
        for(std::size_t i=1;i<active.size();++i)if(sim.attack().controls(sim.blocks().at(active[i]).miner))++own;
        r.share=active.size()>1?double(own)/(active.size()-1):0;
        r.isolated=sim.nodes()[c.attack.target].chain.known_count()==1;
    }
    const auto credits=std::accumulate(sim.pool().shares.begin(),sim.pool().shares.end(),std::uint64_t{0});
    for(std::size_t i=0;i<c.nodes.size();++i)if(sim.attack().controls(i) && credits)r.payout+=r.pool_reward*sim.pool().shares[i]/credits;
    bool connected=false;
    for(const auto& e:sim.trace())if(e.node==c.attack.target){
        if(e.type=="peer_reconnection"){connected=true;++r.reconnections;}
        if(connected && (e.type=="tip_extension" || e.type=="reorganization"))r.recovered=true;
    }
    r.hard_limit=sim.stop_reason()=="duration_ticks" || sim.stop_reason()=="max_events";
    return r;
}
std::string effectiveness(const SimpleResult& a,const SimpleResult& b){
    if(a.attack=="none" || a.attack!=b.attack)return "NON EVALUABLE";
    if(a.outcome=="inconclusive" || b.outcome=="inconclusive" || a.hard_limit || b.hard_limit)return "NON EVALUABLE";
    if(a.attack=="51"){
        if(!a.accepted || !b.accepted || !a.reversed)return "NON EVALUABLE";
        return b.reversed?"NON":"OUI";
    }
    if(a.attack=="selfish"){
        if(a.share<=a.power)return "NON EVALUABLE";
        return b.share<a.share-1e-12?"OUI":"NON";
    }
    if(a.attack=="withholding"){
        if(!a.retained || !b.retained || a.pool_reward<=0 || b.pool_reward<=0)return "NON EVALUABLE";
        return b.payout/b.pool_reward<a.payout/a.pool_reward-1e-12?"OUI":"NON";
    }
    if(!a.isolated)return "NON EVALUABLE";
    return !b.isolated && b.reconnections && b.recovered?"OUI":"NON";
}
void simple_summary(const sim::Config& c,std::ostream& out){
    double total=0,adverse=0;std::size_t miners=0;attacks::Attack roles(c.attack);
    for(std::size_t i=0;i<c.nodes.size();++i){total+=c.nodes[i].hash_power;miners+=c.nodes[i].hash_power>0;if(roles.controls(i))adverse+=c.nodes[i].hash_power;}
    out<<std::defaultfloat<<"\nRecapitulatif : "<<c.nodes.size()<<" noeuds, "<<miners<<" mineurs actifs, hauteur "<<c.target_height<<".\nAttaque : "<<name(c.attack.type)<<'\n';
    if(c.attack.type!="none" && c.attack.type!="sybil")out<<"Puissance adverse : "<<std::setprecision(4)<<100*adverse/total<<" %\n";
    out<<"Securite : "<<(c.security.mode=="protected"?(c.security.detector=="adaptive"?"adaptative":"fixe"):"aucune")<<'\n';
}
void simple_report(const SimpleResult& r,std::ostream& out,const std::string& defense,double honest_pool_reward){
    out<<"\nAttaque choisie : "<<name(r.attack)<<'\n';
    if(r.attack=="sybil")out<<"Puissance adverse : sans objet (identites reseau)\n";
    else if(r.attack!="none")out<<"Puissance adverse : "<<std::fixed<<std::setprecision(2)<<100*r.power<<" %\n";
    out<<"Attaque reussie : "<<(r.outcome=="success"?"OUI":r.outcome=="failed"?"NON":"NON CONCLUANTE")<<'\n';
    if(r.attack=="none")out<<"Simulation de reference, sans attaque.\n";
    else if(r.outcome=="inconclusive")out<<"La tentative reste ouverte ou les limites ne permettent pas de conclure.\n";
    else if(r.attack=="51")out<<(r.reversed?"Un paiement accepte a ete annule.":"Aucun paiement accepte n'a ete renverse dans cet essai.")<<'\n';
    else if(r.attack=="selfish")out<<"Le resultat depend de la part adverse comparee a sa puissance de calcul.\n";
    else if(r.attack=="withholding")out<<"Des blocs peuvent rester retenus meme si la remuneration adverse diminue.\n";
    else out<<(r.isolated?"La victime reste isolee.":"La victime recoit des blocs.")<<'\n';
    out<<"Defense efficace : "<<defense<<'\n';
    if(defense=="NON EVALUABLE")out<<"Une comparaison concluante est necessaire ; un paiement refuse ne prouve pas une protection.\n";
    if(r.attack=="51")out<<"Paiement accepte : "<<yes(r.accepted)<<"\nPaiement annule : "<<yes(r.reversed)<<"\nConfirmations a l'acceptation : "<<r.confirmations<<" / "<<r.required<<'\n';
    else if(r.attack=="selfish")out<<"Part de blocs adverses : "<<std::fixed<<std::setprecision(2)<<100*r.share<<" %\nAvantage sur le hash power : "<<100*(r.share-r.power)<<" points\n";
    else if(r.attack=="withholding"){
        out<<"Blocs retenus : "<<r.retained<<"\nRemuneration adverse : "<<std::fixed<<std::setprecision(2)<<r.payout<<" unites\nPerte de la pool : ";
        if(honest_pool_reward<0)out<<"NON EVALUABLE (reference honnete necessaire)\n";
        else out<<honest_pool_reward-r.pool_reward<<" unites par rapport a la reference honnete\n";
    }else if(r.attack=="sybil")out<<"Victime isolee : "<<yes(r.isolated)<<"\nReconnexions : "<<r.reconnections<<"\nRecuperation observee : "<<yes(r.recovered)<<'\n';
}
void compare_simple(sim::Config c,const std::string& root,std::ostream& out,bool hardened){
    out<<"Simulation en cours...\n"<<std::flush;
    SimpleResult a,b,honest;
    c.security.mode=c.attack.type=="none"?"baseline":"attacked";
    if(hardened)c.security.hardened_security=false;
    execute(c,(std::filesystem::path(root)/"sans_defense").string(),out,false,false,&a,true);
    c.security.reference_file=a.reference_file;c.security.mode=c.attack.type=="none"?"baseline":"protected";
    if(hardened){c.security.hardened_security=true;c.security.reference_file.clear();}
    execute(c,(std::filesystem::path(root)/"avec_defense").string(),out,false,false,&b,true);
    double reference=-1;
    if(c.attack.type=="withholding"){
        c.attack.enabled=false;c.security.mode="baseline";
        execute(c,(std::filesystem::path(root)/"reference_honnete").string(),out,false,false,&honest,true);reference=honest.pool_reward;
    }
    const auto result=effectiveness(a,b);
    out<<"\n--- SANS DEFENSE ---\n";simple_report(a,out,"NON EVALUABLE",reference);
    out<<"\n--- AVEC DEFENSE ---\n";simple_report(b,out,result,reference);
    out<<"Critere de cette comparaison : ";
    if(c.attack.type=="51")out<<"eviter le renversement avec paiement accepte dans les deux essais.";
    else if(c.attack.type=="selfish")out<<"reduire la part adverse initialement avantageuse (attenuation, pas elimination).";
    else if(c.attack.type=="withholding")out<<"reduire la part de remuneration adverse dans la pool (attenuation, blocs non restitues).";
    else if(c.attack.type=="sybil")out<<"retablir une adoption apres reconnexion lorsque la victime etait isolee sans defense.";
    else out<<"aucune attaque a evaluer.";
    out<<"\nMeme configuration et seed ; seules les defenses changent. Resultat limite a cet essai.\nExports JSON : "<<root<<'\n';
}
}
