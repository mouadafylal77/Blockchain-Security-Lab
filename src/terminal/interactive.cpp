#include "terminal/interactive.hpp"
#include "terminal/display.hpp"
#include "terminal/simple.hpp"
#include "sim/rng.hpp"
#include "sim/simulator.hpp"
#include "metrics/json_exporter.hpp"
#include "detection/config_io.hpp"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <set>
#include <sstream>
#include <stdexcept>
#include <type_traits>

namespace bsl::terminal {
namespace {
struct Back {};
struct Cancel {};
class Form {
    std::istream& in_;std::ostream& out_;
    std::vector<std::string> answers_;
    std::vector<std::string> labels_;
    std::size_t cursor_=0;
public:
    Form(std::istream& in,std::ostream& out):in_(in),out_(out){}
    void replay(){cursor_=0;}
    void back(){if(!answers_.empty()){answers_.pop_back();labels_.pop_back();}}
    void section(const std::string& title){if(cursor_>=answers_.size())out_<<"\n----------- "<<title<<" -----------\n\n";}
    int menu(const std::string& title,const std::vector<std::string>& options,int fallback,int first=0){
        if(cursor_>=answers_.size()){out_<<title<<"\n\n";for(std::size_t i=0;i<options.size();++i)out_<<"  "<<i+first<<". "<<options[i]<<'\n';out_<<'\n';}
        return number<int>("Votre choix",fallback,first,first+static_cast<int>(options.size())-1);
    }
    void revise(){
        out_<<"\nQuel parametre souhaitez-vous corriger ?\n";
        for(std::size_t i=0;i<labels_.size();++i)out_<<i+1<<". "<<labels_[i]<<" : "<<answers_[i]<<'\n';
        for(;;){
            out_<<"Numero du parametre (b = dernier, q = annuler) : ";std::string text;
            if(!std::getline(in_,text) || text=="q" || text=="annuler")throw Cancel{};
            if(text=="b" || text=="retour"){back();throw Back{};}
            std::istringstream parser(text);std::size_t index=0;std::string extra;
            if((parser>>index) && !(parser>>extra) && index>=1 && index<=answers_.size()){
                answers_.resize(index-1);labels_.resize(index-1);throw Back{};
            }
            out_<<"Numero invalide.\n";
        }
    }
    template<class T>T number(const std::string& prompt,T fallback,T low,T high){
        for(;;){
            std::string text;
            const bool cached=cursor_<answers_.size();
            if(cached)text=answers_[cursor_];
            else{
                out_<<prompt<<" : "<<std::flush;
                if(!std::getline(in_,text))throw Cancel{};
                if(text=="q" || text=="annuler")throw Cancel{};
                if(text=="b" || text=="retour"){back();throw Back{};}
                if(text=="?" || text=="aide"){
                    out_<<"Valeur proposee : "<<fallback<<" ; valeurs autorisees : "<<low<<" a "<<high
                        <<". Entree accepte la proposition. b = retour, q = annuler.\n";continue;
                }
                if(text.empty()){std::ostringstream value;value<<std::setprecision(17)<<fallback;text=value.str();}
            }
            std::istringstream parser(text);T value{};
            const bool parsed=static_cast<bool>(parser>>value);
            std::string tail;const bool extra=static_cast<bool>(parser>>tail);
            const bool negative_unsigned=std::is_unsigned<T>::value && text.find('-')!=std::string::npos;
            if(parsed && !extra && !negative_unsigned && std::isfinite(static_cast<double>(value)) && value>=low && value<=high){
                if(!cached){answers_.push_back(text);labels_.push_back(prompt);}
                ++cursor_;return value;
            }
            if(cached){answers_.resize(cursor_);labels_.resize(cursor_);}
            out_<<"Entree invalide : nombre entre "<<low<<" et "<<high<<" attendu (? = aide).\n";
        }
    }
    bool flag(const std::string& label,bool value){return number<int>(label,value?1:0,0,1)!=0;}
};
const std::vector<std::string> kinds{"none","51","selfish","withholding","sybil"};
void save(const std::filesystem::path& path,const std::string& text){
    std::ofstream file(path,std::ios::binary);if(!file || !(file<<text))throw std::runtime_error("Cannot export "+path.string());
}
std::filesystem::path fresh_directory(){
    for(unsigned n=1;n<100000;++n){
        const auto p=std::filesystem::path("results/raw/interactive")/("run_"+std::to_string(n));
        if(!std::filesystem::exists(p))return p;
    }
    throw std::runtime_error("No free run directory");
}
}
sim::Config generate(const Request& r){
    if(r.nodes<2 || r.nodes>256 || !r.miners || r.miners>r.nodes || !r.height || r.height>10000
        || std::find(kinds.begin(),kinds.end(),r.attack)==kinds.end())throw std::invalid_argument("Invalid scenario dimensions/attack");
    if(r.attack=="sybil" && r.nodes<r.miners+2)throw std::invalid_argument("Sybil : reserver une cible passive et au moins une identite sans hash power (noeuds >= mineurs+2)");
    sim::Config c;
    c.scenario="interactive_"+r.attack;c.seed=r.seed;c.target_height=r.height;c.observer=r.nodes-1;
    c.max_blocks=std::min<std::uint64_t>(10000,std::max<std::uint64_t>(100,r.height*20));
    c.max_events=c.max_pending_events=100000;c.duration_ticks=1000000;
    c.mean_block_ticks=r.attack=="withholding"?200:100;c.difficulty=r.attack=="withholding"?2:1;
    sim::Rng rng(r.seed^0x94d049bb133111ebULL);
    for(std::size_t i=0;i<r.nodes;++i)c.nodes.push_back({"node"+std::to_string(i),i<r.miners?double(10+rng.uniform_ticks(90)):0,"group"+std::to_string(i)});
    if(r.adversary_percent && (r.attack=="51" || r.attack=="selfish" || r.attack=="withholding")){
        const double q=*r.adversary_percent;
        if(!std::isfinite(q) || q<0 || q>100 || (r.miners<2 && q<100))throw std::invalid_argument("Pour repartir la puissance honnete, choisir au moins deux mineurs (ou 100 % adverse)");
        double honest_weight=0;for(std::size_t i=1;i<r.miners;++i)honest_weight+=c.nodes[i].hash_power;
        c.nodes[0].hash_power=q;
        for(std::size_t i=1;i<r.miners;++i)c.nodes[i].hash_power=(100-q)*c.nodes[i].hash_power/honest_weight;
    }
    std::set<std::pair<std::size_t,std::size_t>> links;
    for(std::size_t i=0;i<r.nodes;++i){const auto next=(i+1)%r.nodes;auto edge=std::minmax(i,next);if(links.insert(edge).second)c.links.push_back({edge.first,edge.second,5,2});}
    c.attack.type=r.attack;c.attack.target=r.nodes-1;c.attack.adversary=0;c.attack.public_notice=true;
    if(r.attack!="none")c.attack.adversaries={0};
    if(r.attack=="withholding"){
        if(r.pool_honest_miners>=r.miners)throw std::invalid_argument("Too many honest pool miners");
        const auto count=r.pool_honest_miners?r.pool_honest_miners:r.miners-1;
        for(std::size_t i=0;i<=count;++i)c.attack.pool_members.push_back(i);
    }
    if(r.attack=="sybil"){
        if(r.sybil_count>r.nodes-r.miners-1)throw std::invalid_argument("Too many Sybil identities");
        const auto count=r.sybil_count?r.sybil_count:r.nodes-r.miners-1;
        for(std::size_t i=r.miners;i<r.miners+count;++i)c.attack.sybil_identities.push_back(i);
    }
    c.security.mode=r.attack=="none"?"baseline":r.defenses?"protected":"attacked";
    c.security.audit_ticks=10;c.security.event_audits=true;
    c.security.detector=r.detector;
    c.security.mitigate_confirmations=r.attack=="51";c.security.mitigate_publication=r.attack=="selfish";
    c.security.mitigate_pool=r.attack=="withholding";c.security.mitigate_peers=r.attack=="sybil";
    if(r.attack=="withholding")c.security.window_ticks=10000;
    if(r.attack=="selfish")c.security.publication_delay=100;
    sim::validate_config(c);return c;
}
sim::Config configure(std::istream& input,std::ostream& output,bool* detailed,bool* simple){
    Form f(input,output);
    output<<"========================================\n       BLOCKCHAIN SECURITY LAB\n         Simulation Blockchain PoW\n========================================\n\n"
        <<"Entree accepte la valeur proposee. ?/aide affiche proposition et bornes.\n"
        <<"b/retour : question precedente ; q/annuler : quitter.\n\n";
    for(;;){try{
        f.replay();
        const bool custom=f.menu("Choisissez votre mode :",{"Mode simple","Mode personnalise"},1,1)==2;
        f.section("CONFIGURATION");
        Request r;
        r.nodes=f.number<std::size_t>("Nombre de noeuds",4,2,256);
        r.miners=f.number<std::size_t>("Nombre de mineurs",std::min<std::size_t>(2,r.nodes),1,r.nodes);
        r.height=f.number<std::uint64_t>("Hauteur cible de la blockchain (vue locale)",20,1,10000);
        f.section("CHOIX DE L'ATTAQUE");
        r.attack=kinds.at(f.menu("Choisissez une attaque :",{"Aucune attaque","Double depense (attaque 51 %)","Selfish Mining","Block Withholding","Sybil"},0));
        if(r.attack=="sybil" && r.nodes<r.miners+2){output<<"Sybil exige noeuds >= mineurs+2. Modifier l'attaque ou revenir avec b.\n";f.back();throw Back{};}
        if(!custom && (r.attack=="51" || r.attack=="selfish" || r.attack=="withholding")){
            f.section("RESSOURCES ADVERSES");
            r.adversary_percent=f.number<double>(r.attack=="withholding"?"Hash power du mineur infiltre (%)":"Hash power de l'attaquant (%)",40,0,100);
            if(r.miners<2 && *r.adversary_percent<100){output<<"Le reste de la puissance exige un mineur honnete. Corriger la puissance ou revenir aux mineurs.\n";f.back();throw Back{};}
        }
        f.section("SECURITE");
        const auto security=f.menu("Choisissez la protection :",{"Aucune","Fixe","Adaptative"},0);
        r.defenses=security!=0;r.detector=security==2?"adaptive":"fixed";
        if(custom)r.seed=f.number<std::uint64_t>("Seed",42,0,UINT64_MAX);
        auto c=generate(r);
        if(custom){
            c.attack.adversaries.clear();
            for(std::size_t i=0;i<r.miners;++i){
                c.nodes[i].hash_power=f.number<double>("Hash power de node"+std::to_string(i),c.nodes[i].hash_power,0,1e12);
                if(r.attack!="none" && r.attack!="sybil" && f.flag("Mineur malveillant node"+std::to_string(i),i==0))c.attack.adversaries.push_back(i);
            }
            if(r.attack!="none"){
                if(r.attack=="sybil")c.attack.adversaries={0};
                if(c.attack.adversaries.empty()){output<<"Selectionner au moins un mineur adverse (sa puissance peut etre nulle).\n";f.back();throw Back{};}
                c.attack.adversary=c.attack.adversaries.front();
                const auto target=f.number<std::size_t>("Noeud cible/observateur",r.nodes-1,0,r.nodes-1);
                if(std::find(c.attack.adversaries.begin(),c.attack.adversaries.end(),target)!=c.attack.adversaries.end()
                    || (r.attack=="sybil" && target<r.miners)){output<<"Cible incompatible avec les roles.\n";f.back();throw Back{};}
                c.attack.target=c.observer=target;
            }else c.observer=f.number<std::size_t>("Noeud observateur",r.nodes-1,0,r.nodes-1);
            const auto topology=f.number<int>("Topologie 1=anneau, 2=complete, 3=ligne, 4=liens manuels",1,1,4);
            const auto latency=f.number<sim::Time>("Latence (ticks)",5,0,1000000000);
            const auto jitter=f.number<sim::Time>("Jitter maximal (ticks)",2,0,1000000000);
            c.links.clear();
            if(topology==4){
                const auto count=f.number<std::size_t>("Nombre de liens (partition autorisee)",0,0,r.nodes*(r.nodes-1)/2);
                std::set<std::pair<std::size_t,std::size_t>> edges;
                for(std::size_t i=0;i<count;++i){
                    const auto a=f.number<std::size_t>("Lien "+std::to_string(i)+" origine",0,0,r.nodes-1);
                    const auto b=f.number<std::size_t>("Destination",1,0,r.nodes-1);
                    if(a==b || !edges.insert(std::minmax(a,b)).second){output<<"Lien duplique ou boucle.\n";f.back();throw Back{};}
                    auto l=f.number<sim::Time>("Latence de ce lien",latency,0,1000000000);
                    auto j=f.number<sim::Time>("Jitter de ce lien",jitter,0,1000000000);
                    c.links.push_back({a,b,l,j});
                }
            }else for(std::size_t i=0;i<r.nodes;++i)for(std::size_t j=i+1;j<r.nodes;++j)
                if(topology==2 || j==i+1 || (topology==1 && i==0 && j==r.nodes-1))c.links.push_back({i,j,latency,jitter});
            if(r.attack=="51")c.attack.confirmations=f.number<unsigned>("Confirmations marchandes (inclusion comprise)",2,1,10000);
            if(r.attack=="withholding"){
                c.attack.pool_members.clear();
                for(std::size_t i=0;i<r.miners;++i)if(std::find(c.attack.adversaries.begin(),c.attack.adversaries.end(),i)!=c.attack.adversaries.end()
                    || f.flag("node"+std::to_string(i)+" membre de pool",true))c.attack.pool_members.push_back(i);
            }
            if(r.attack=="sybil"){
                c.attack.sybil_identities.clear();
                for(std::size_t i=r.miners;i<r.nodes;++i)if(i!=c.attack.target && f.flag("node"+std::to_string(i)+" identite Sybil",true))c.attack.sybil_identities.push_back(i);
                c.attack.capture_connections=f.flag("Capture initiale des connexions (hypothese forte)",true);
                output<<"Les groupes administratifs sont supposes observables/fiables ; un groupe distinct ne prouve pas l'honnetete.\n";
                for(std::size_t i=0;i<c.nodes.size();++i)c.nodes[i].peer_group="group"+std::to_string(f.number<unsigned>("Groupe administratif de node"+std::to_string(i),static_cast<unsigned>(i),0,255));
            }
            c.mean_block_ticks=f.number<double>("Intervalle global moyen (ticks)",c.mean_block_ticks,1,1e9);
            c.difficulty=f.number<unsigned>("Difficulte PoW",c.difficulty,r.attack=="withholding"?2:1,6);
            c.mining_attempts=f.number<std::uint64_t>("Budget SHA-256 par bloc",1000000,1,10000000);
            c.duration_ticks=f.number<sim::Time>("Horizon maximal (ticks)",1000000,1,1000000000000ULL);
            c.max_blocks=f.number<std::uint64_t>("Plafond de decouvertes, distinct de la hauteur cible",c.max_blocks,1,10000);
            c.max_events=f.number<std::uint64_t>("Evenements maximum",100000,1,1000000);
            c.max_pending_events=f.number<std::uint64_t>("Capacite de file",100000,1,1000000);
            auto& s=c.security;
            const auto detector=f.number<int>("Detecteur 0=aucun, 1=fixe, 2=adaptatif",s.detector=="adaptive"?2:1,r.defenses?1:0,2);
            if(detector==0)s.mode="off";
            else{
                s.detector=detector==1?"fixed":"adaptive";
                s.window_ticks=f.number<sim::Time>("Fenetre d'observation (ticks)",s.window_ticks,1,1000000000);
                s.audit_ticks=f.number<sim::Time>("Intervalle d'audit",std::min(s.audit_ticks,s.window_ticks),1,s.window_ticks);
                s.cooldown_ticks=f.number<sim::Time>("Intervalle minimum entre alertes identiques",500,1,1000000000);
                if(detector==2){
                    s.alpha=f.number<double>("EWMA alpha",0.1,0.000001,1);
                    s.k=f.number<double>("Facteur de dispersion k",3,0.000001,20);
                    s.clip=f.number<double>("Borne de mise a jour clip",2,0.000001,10);
                    s.drift_cap=f.number<double>("Derive maximale drift_cap",2,0.000001,10);
                    s.min_samples=f.number<std::uint64_t>("Echantillons minimum de calibration honnete",5,2,100000);
                }
                std::vector<std::string> metrics;
                if(r.attack=="51")metrics={"reorg_depth","fork_rate","silence_ticks"};
                if(r.attack=="selfish")metrics={"late_p95","stale_ratio","fork_rate"};
                if(r.attack=="withholding")metrics={"share_deficit"};
                if(r.attack=="sybil")metrics={"peer_concentration","silence_ticks"};
                for(const auto& metric:metrics)s.thresholds[metric]=f.number<double>("Seuil "+metric,s.thresholds[metric],0.000001,1e15);
                if(r.attack=="withholding")s.min_shares=f.number<std::uint64_t>("Shares minimum pour audit",64,1,1000000);
                if(r.defenses){
                    s.hold_ticks=f.number<sim::Time>("Duree de suspension/probation (ticks)",500,1,1000000000);
                    s.confirmation_ticks=std::max(s.hold_ticks,s.confirmation_ticks);
                    if(r.attack=="51"){
                        s.confirmation_ticks=f.number<sim::Time>("Duree des confirmations renforcees",std::max<sim::Time>(2000,s.hold_ticks),s.hold_ticks,1000000000);
                        s.confirmation_increment=f.number<unsigned>("Increment de confirmations",3,1,10000);
                        s.max_confirmations=f.number<unsigned>("Plafond de confirmations",std::max(100u,s.confirmation_increment),s.confirmation_increment,10000);
                        s.proactive_signals=f.flag("Reagir aussi aux forks/silences publics (risque de faux positifs)",false);
                        s.event_audits=f.flag("Audits immediats sur forks/reorganisations",true);
                    }
                    if(r.attack=="selfish")s.publication_delay=f.number<sim::Time>("Retard de probation",100,1,1000000000);
                    if(r.attack=="sybil"){
                        s.max_peers=f.number<unsigned>("Pairs maximum",4,1,256);
                        s.max_per_group=f.number<unsigned>("Pairs maximum par groupe administratif",1,1,s.max_peers);
                    }
                }
            }
        }
        if(!custom)c.security.hardened_security=r.defenses;
        try{sim::validate_config(c);}catch(const std::exception& e){output<<"Configuration invalide : "<<e.what()<<". Retour a la derniere question.\n";f.back();throw Back{};}
        bool detail=false;
        if(custom){
            f.section("FIN D'EXPERIENCE");
            c.post_target_ticks=f.number<sim::Time>("Observation apres hauteur cible (ticks de production supplementaires, 0=arret immediat)",static_cast<sim::Time>(20*c.mean_block_ticks),0,1000000000);
            detail=f.flag("Afficher les evenements detailles (0=synthetique, 1=detaille)",false);
            summary(c,output);
        }else{
            c.post_target_ticks=static_cast<sim::Time>(20*c.mean_block_ticks);
            simple_summary(c,output);
        }
        if(f.menu("Avant de lancer :",{"Lancer la simulation","Corriger un parametre"},1,1)==2){f.back();f.revise();}
        if(detailed)*detailed=detail;
        if(simple)*simple=!custom;
        return c;
    }catch(const Back&){continue;}}
}
void summary(const sim::Config& c,std::ostream& out){
    double total=0,adverse=0,pool=0;std::size_t miners=0;attacks::Attack roles(c.attack);
    for(std::size_t i=0;i<c.nodes.size();++i){total+=c.nodes[i].hash_power;miners+=c.nodes[i].hash_power>0;
        if(c.attack.type!="none" && roles.controls(i))adverse+=c.nodes[i].hash_power;
        if(std::find(c.attack.pool_members.begin(),c.attack.pool_members.end(),i)!=c.attack.pool_members.end())pool+=c.nodes[i].hash_power;
    }
    out<<"\n=========== RECAPITULATIF ===========\n"
       <<"Noeuds : "<<c.nodes.size()<<" ; mineurs actifs : "<<miners<<" ; observateur : "<<c.nodes[c.observer].id<<'\n';
    out<<std::setprecision(6)<<"Ressources (poids relatifs ; total "<<total<<") :\n";
    for(const auto& n:c.nodes)if(n.hash_power>0)out<<"  "<<n.id<<" : "<<n.hash_power<<'\n';
    if(c.attack.type!="none" && c.attack.type!="sybil")out<<"Hash power attaquant : "<<100*adverse/total<<" %\nHash power honnete : "<<100*(total-adverse)/total<<" %\n"
        <<"Cette repartition influence les decouvertes, sans garantir le succes.\n";
    std::vector<std::size_t> degree(c.nodes.size());std::vector<std::vector<std::size_t>> adjacent(c.nodes.size());
    for(const auto& l:c.links){++degree[l.from];++degree[l.to];adjacent[l.from].push_back(l.to);adjacent[l.to].push_back(l.from);}
    std::vector<bool> visited(c.nodes.size(),false);std::vector<std::size_t> pending{0};visited[0]=true;
    for(std::size_t i=0;i<pending.size();++i)for(const auto peer:adjacent[pending[i]])if(!visited[peer]){visited[peer]=true;pending.push_back(peer);}
    const bool connected=pending.size()==c.nodes.size();
    std::string topology="personnalisee";
    if(connected && c.links.size()==c.nodes.size()-1 && std::count(degree.begin(),degree.end(),1)==2 && std::all_of(degree.begin(),degree.end(),[](auto n){return n==1 || n==2;}))topology="ligne";
    if(connected && c.links.size()==c.nodes.size() && std::all_of(degree.begin(),degree.end(),[](auto n){return n==2;}))topology="anneau";
    if(c.links.size()==c.nodes.size()*(c.nodes.size()-1)/2)topology="complete";
    out<<"Topologie : "<<topology<<" ; liens : "<<c.links.size();
    if(!connected)out<<" ; reseau partitionne";
    if(!c.links.empty()){
        sim::Time minlat=c.links.front().latency,maxlat=minlat,maxj=0;
        for(const auto& l:c.links){minlat=std::min(minlat,l.latency);maxlat=std::max(maxlat,l.latency);maxj=std::max(maxj,l.jitter);}
        out<<" ; latence "<<minlat<<".."<<maxlat<<" ticks ; jitter maximal "<<maxj;
    }
    out<<"\nAttaque : "<<c.attack.type<<" ; strategie M3 inchangee\n";
    if(c.attack.type=="51")out<<"Confirmations marchandes : "<<c.attack.confirmations<<" (inclusion comprise)\n";
    if(c.attack.type=="withholding")out<<"Pool : "<<c.attack.pool_members.size()<<" membres, "<<100*pool/total<<" % du hash power total ; shares distinctes des blocs complets\n";
    if(c.attack.type=="sybil")out<<"Identites Sybil : "<<c.attack.sybil_identities.size()<<" ; aucune puissance ajoutee ; capture initiale : "<<(c.attack.capture_connections?"oui":"non")<<'\n';
    out<<"Detection : "<<(c.security.mode=="off"?"aucune":c.security.detector)<<" ; mitigation : "<<(c.security.mode=="protected"?"active":"inactive")<<'\n'
       <<"Calibration : "<<(c.security.detector=="adaptive"?"reference honnete separee avant execution":"aucune (seuils fixes non calibres pour ce reseau)")<<'\n'
       <<"Hauteur cible locale : "<<c.target_height<<" ; seed : "<<c.seed<<'\n'
       <<"Apres la cible : "<<c.post_target_ticks<<" ticks de production normale, puis livraison des messages ; aucune publication forcee\n"
       <<"Limites : "<<c.duration_ticks<<" ticks, "<<c.max_blocks<<" decouvertes, "<<c.max_events<<" evenements, file "<<c.max_pending_events<<"\n\n";
}
void execute(sim::Config c,const std::string& directory,std::ostream& output,bool pedagogical,bool detailed,SimpleResult* result,bool quiet){
    std::ostringstream muted;auto& out=quiet?static_cast<std::ostream&>(muted):output;
    const std::filesystem::path path(directory);
    metrics::validate_output_directory(directory);
    if(std::filesystem::exists(path))throw std::invalid_argument("Run directory already exists; nothing overwritten");
    std::filesystem::create_directories(path);
    if(c.security.detector=="adaptive" && c.security.mode!="off" && c.security.mode!="calibration" && c.security.reference_file.empty()){
        auto honest=c;honest.attack.enabled=false;honest.security.mode="calibration";
        honest.seed=c.seed^0xd6e8feb86659fd93ULL;honest.target_height=0;
        honest.max_blocks=std::max<std::uint64_t>(100,c.max_blocks);honest.security.reference_file.clear();
        out<<"Calibration honnete separee, seed="<<honest.seed<<" (aucune observation attaquee).\n";
        sim::Simulator calibration(honest);calibration.run();
        if(calibration.detector().calibration().empty())throw std::runtime_error("Calibration insuffisante : augmenter horizon/budgets");
        metrics::write_results(calibration,(path/"calibration").string());
        c.security.reference_file=std::filesystem::absolute(path/"calibration/calibration.json").generic_string();
        if(detection::load_reference(c.security).empty())throw std::runtime_error("Calibration insuffisante : aucun seuil ne dispose de min_samples observations");
    }
    save(path/"config.json",sim::config_json(c));
    sim::Simulator sim(c);
    std::optional<Display> display;
    if(pedagogical){display.emplace(sim,out,detailed);sim.set_observer([&](const sim::TraceRecord& event){display->event(event);});}
    sim.run();metrics::write_results(sim,directory);
    if(pedagogical)display->finish();
    if(result)*result=simple_result(sim);
    const auto outcome=sim.outcome();
    out<<"Etat final : "<<(outcome=="success"?"attaque reussie":outcome=="failed"?"attaque echouee":outcome=="inconclusive"?"resultat non concluant":"reference honnete")
       <<" ; stop="<<sim.stop_reason()<<" ; hauteur="<<sim.nodes()[c.observer].chain.tip().index
       <<" ; decouvertes="<<sim.metrics().blocks_produced<<" ; forks="<<sim.metrics().forks<<" ; reorganisations="<<sim.metrics().reorganizations
       <<" ; alertes="<<sim.detector().alerts().size()<<" ; actions="<<sim.defenses().actions<<'\n';
    if(c.attack.type=="51")out<<"Paiement accepte="<<sim.attack().accepted<<" ; confirmations observees/requises="<<sim.attack().accepted_confirmations<<'/'<<sim.attack().accepted_required<<". Non-acceptation != defense reussie.\n";
    out<<"Motif : "<<sim.outcome_reason()<<" ; limite d'execution atteinte : "
       <<((sim.stop_reason()=="duration_ticks" || sim.stop_reason()=="max_events" || sim.stop_reason()=="max_blocks_drained")?"oui":"non")
       <<" ; fin de production au tick "<<sim.production_end_tick()<<'\n';
    out<<"Exports : "<<directory<<"/config.json et results.json\n";
}
int interactive(std::istream& in,std::ostream& out){
    try{
        for(;;){
            bool detailed=false,simple=false;const auto c=configure(in,out,&detailed,&simple);
            bool first=true;
            for(;;){
                if(first){
                    if(simple){SimpleResult result;out<<"Simulation en cours...\n"<<std::flush;execute(c,fresh_directory().string(),out,false,false,&result,true);simple_report(result,out);}
                    else execute(c,fresh_directory().string(),out,true,detailed);
                    first=false;
                }
                out<<"\n1. Nouvelle simulation\n2. Rejouer\n3. Comparer sans/avec defense\n0. Quitter\nVotre choix : "<<std::flush;
                std::string choice;if(!std::getline(in,choice) || choice=="0" || choice=="q")return 0;
                if(choice=="1")break;
                if(choice=="2"){first=true;continue;}
                if(choice=="3"){compare_simple(c,fresh_directory().string(),out,simple);continue;}
                out<<"Choix invalide.\n";
            }
        }
    }catch(const Cancel&){out<<"Configuration annulee.\n";return 0;}
}
}
