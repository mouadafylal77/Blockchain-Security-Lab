"""Rebuild all M5 statistics/figures from lightweight, hash-validated summaries."""
from __future__ import annotations

import csv
import hashlib
import json
import math
import os
from pathlib import Path
import statistics
import sys

import numpy as np

import benchmark as b

OUT=b.ROOT/"analysis"
TABLES=OUT/"tables"
FIGURES=OUT/"figures"
PROTOCOL=b.read(b.PROTOCOL)

def rng_for(key):
    seed=int.from_bytes(hashlib.sha256(f"{PROTOCOL['bootstrap_seed']}:{key}".encode()).digest()[:8],"big")
    return np.random.default_rng(seed)

def describe(values,key):
    vals=np.asarray([v for v in values if v is not None],dtype=float)
    if not np.all(np.isfinite(vals)):raise ValueError("Nonfinite statistical input")
    n=len(vals)
    if not n:return {"n":0,"missing":len(values),**{k:None for k in ("mean","median","sd","min","max","mean_ci_low","mean_ci_high","median_ci_low","median_ci_high")}}
    if n==1:
        return {"n":1,"missing":len(values)-1,"mean":float(vals[0]),"median":float(vals[0]),"sd":None,"min":float(vals[0]),"max":float(vals[0]),"mean_ci_low":None,"mean_ci_high":None,"median_ci_low":None,"median_ci_high":None}
    indices=rng_for(key).integers(0,n,size=(PROTOCOL["bootstrap_replicates"],n))
    boot=vals[indices]
    mean_ci=np.quantile(boot.mean(axis=1),[.025,.975])
    median_ci=np.quantile(np.median(boot,axis=1),[.025,.975])
    return {"n":n,"missing":len(values)-n,"mean":float(vals.mean()),"median":float(np.median(vals)),
        "sd":float(vals.std(ddof=1)) if n>1 else None,"min":float(vals.min()),"max":float(vals.max()),
        "mean_ci_low":float(mean_ci[0]),"mean_ci_high":float(mean_ci[1]),"median_ci_low":float(median_ci[0]),"median_ci_high":float(median_ci[1])}

def wilson(success,n):
    if not n:return None,None
    p=success/n;z=1.959963984540054;denominator=1+z*z/n
    midpoint=(p+z*z/(2*n))/denominator
    half=z*math.sqrt(p*(1-p)/n+z*z/(4*n*n))/denominator
    return max(0,midpoint-half),min(1,midpoint+half)

def micro_detection(rows,key):
    counts=np.asarray([[r[k] for k in ("tp","fp","tn","fn")] for r in rows],dtype=float)
    sums=counts.sum(axis=0)
    indices=rng_for(key).integers(0,len(rows),size=(PROTOCOL["bootstrap_replicates"],len(rows)))
    resampled=counts[indices].sum(axis=1)
    def scores(array):
        tp,fp,tn,fn=array.T
        p=np.divide(tp,tp+fp,out=np.zeros_like(tp),where=tp+fp!=0)
        r=np.divide(tp,tp+fn,out=np.zeros_like(tp),where=tp+fn!=0)
        f=np.divide(2*p*r,p+r,out=np.zeros_like(tp),where=p+r!=0)
        return p,r,f
    point=scores(sums[None,:]);boot=scores(resampled)
    result={k:int(v) for k,v in zip(("tp","fp","tn","fn"),sums)}
    for name,values,est in zip(("precision","recall","f1"),boot,point):
        ci=np.quantile(values,[.025,.975]);result.update({"micro_"+name:float(est[0]),name+"_ci_low":float(ci[0]),name+"_ci_high":float(ci[1])})
    return result

def save_csv(path,rows):
    path.parent.mkdir(parents=True,exist_ok=True)
    if not rows:raise ValueError("Refusing empty table")
    columns=list(rows[0])
    with path.open("w",newline="",encoding="utf-8") as f:
        writer=csv.DictWriter(f,fieldnames=columns);writer.writeheader();writer.writerows(rows)

def load_summaries():
    b.verify_reference();records=b.latest_records(b.DEFAULT_OUTPUT);rows=[]
    expected=list(j for j in b.jobs(PROTOCOL,"evaluation") if j[0]=="evaluation")
    for phase,attack,detector,mode,seed,suffix in expected:
        job="/".join(map(str,(phase,attack,detector,mode,seed)))
        r=records.get(job)
        if not r or r["status"]!="valid":raise RuntimeError(f"Campaign incomplete: {job}")
        for field in ("config","result","summary"):
            if b.digest(b.ROOT/r[field+"_path"])!=r[field+"_sha256"]:raise RuntimeError(f"Artifact integrity error: {job}, {field}")
        row=b.read(b.ROOT/r["summary_path"])
        if row["seed"]!=seed or row["phase"]!="evaluation":raise ValueError("Wrong summary assignment")
        rows.append(row)
    if len(rows)!=720:raise RuntimeError("Expected exactly 720 evaluated runs")
    # Paired detector-only comparisons must retain identical stochastic block processes when defenses are off.
    lookup={(r["attack"],r["detector"],r["mode"],r["seed"]):r for r in rows}
    for attack in PROTOCOL["attacks"]:
        for mode in ("baseline","attacked"):
            for seed in PROTOCOL["evaluation_seeds"]:
                if lookup[attack,"fixed",mode,seed]["process_signature"]!=lookup[attack,"adaptive",mode,seed]["process_signature"]:
                    raise RuntimeError("Detector-only comparison unexpectedly changed the block process; investigate without changing M4")
    return rows,records

def aggregate(rows):
    numeric_keys=[k for k,v in rows[0].items() if isinstance(v,(int,float)) and k!="seed"]
    for r in rows:
        for k,v in r.items():
            if isinstance(v,(int,float)) and k!="seed" and k not in numeric_keys:numeric_keys.append(k)
    numeric_keys=[k for k in numeric_keys if k not in ("rss_samples",)]
    for k in ("latency_ticks","accepted_at","adversary_payout","honest_payout","pool_full_withheld","shares_observed","honest_shares_uncredited","recovery_after_reconnect_ticks","confirmations_at_acceptance"):
        if k not in numeric_keys:numeric_keys.append(k)
    statistics_rows=[];groups=[];grouped={};stats={}
    for attack in PROTOCOL["attacks"]:
        for detector in PROTOCOL["detectors"]:
            for mode in PROTOCOL["modes"]:
                key=(attack,detector,mode);group=sorted([r for r in rows if (r["attack"],r["detector"],r["mode"])==key],key=lambda r:r["seed"])
                if len(group)!=30 or [r["seed"] for r in group]!=PROTOCOL["evaluation_seeds"]:raise ValueError("Noncompliant cohort")
                grouped[key]=group
                g={"attack":attack,"detector":detector,"mode":mode,"valid_n":len(group),"invalid_n":0}
                g.update(micro_detection(group,str(key)))
                for metric in numeric_keys:
                    d=describe([r.get(metric) for r in group],f"{key}:{metric}");stats[key,metric]=d
                    statistics_rows.append({"attack":attack,"detector":detector,"mode":mode,"metric":metric,**d})
                success=stats[key,"attack_success"]
                g.update(success_rate=success["mean"],success_bootstrap_low=success["mean_ci_low"],success_bootstrap_high=success["mean_ci_high"])
                g["success_wilson_low"],g["success_wilson_high"]=wilson(sum(r["attack_success"] for r in group),len(group))
                for metric in ("latency_ticks","payment_loss","adversary_active_share","target_stale_ratio","pool_reward","adversary_payout","honest_payout","messages","events","child_cpu_seconds","wall_seconds"):
                    g[metric+"_mean"]=stats[key,metric]["mean"]
                    if metric=="latency_ticks":g["detected_n"]=stats[key,metric]["n"];g["undetected_or_inapplicable_n"]=stats[key,metric]["missing"]
                g["honest_sanction_runs"]=sum((r.get("honest_shares_uncredited") or 0)>0 for r in group)
                g["censored_by_budget_n"]=sum(r["stop_reason"]!="max_blocks_drained" or r["pending_events"]>0 for r in group)
                groups.append(g)
    save_csv(TABLES/"m5_runs.csv",rows)
    save_csv(TABLES/"m5_groups.csv",groups)
    save_csv(TABLES/"m5_statistics.csv",statistics_rows)
    pairs=[]
    for attack in PROTOCOL["attacks"]:
        for detector in PROTOCOL["detectors"]:
            for left,right,label in (("attacked","protected","protected_minus_attacked"),("baseline","attacked","attacked_minus_baseline")):
                a=grouped[attack,detector,left];bb=grouped[attack,detector,right]
                for metric in numeric_keys:
                    values=[y[metric]-x[metric] if x.get(metric) is not None and y.get(metric) is not None else None for x,y in zip(a,bb)]
                    pairs.append({"attack":attack,"detector":detector,"comparison":label,"metric":metric,**describe(values,f"paired:{attack}:{detector}:{label}:{metric}")})
    for attack in PROTOCOL["attacks"]:
        for mode in PROTOCOL["modes"]:
            for metric in numeric_keys:
                a=grouped[attack,"fixed",mode];bb=grouped[attack,"adaptive",mode]
                values=[y[metric]-x[metric] if x.get(metric) is not None and y.get(metric) is not None else None for x,y in zip(a,bb)]
                pairs.append({"attack":attack,"detector":"adaptive_vs_fixed","comparison":f"adaptive_minus_fixed_{mode}","metric":metric,**describe(values,f"detectors:{attack}:{mode}:{metric}")})
    save_csv(TABLES/"m5_paired.csv",pairs)
    return groups,stats,grouped,pairs

def figures(groups,stats,grouped):
    # Optional local plotting wheels, without modifying the system interpreter or M4.
    sys.path.insert(0,str(b.ROOT/"build/m5-python"))
    os.environ["MPLCONFIGDIR"]=str(b.ROOT/"build/m5-matplotlib-cache")
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    from matplotlib.ticker import PercentFormatter
    FIGURES.mkdir(parents=True,exist_ok=True)
    plt.rcParams.update({"font.size":10,"axes.spines.top":False,"axes.spines.right":False,"figure.facecolor":"white","savefig.facecolor":"white"})
    names={"attack_51":"Double dépense / 51 %","selfish_mining":"Selfish mining","block_withholding":"Block withholding","sybil":"Sybil"}
    colors={"fixed":"#20639b","adaptive":"#b54c23"}
    def save(fig,name):
        fig.tight_layout();fig.savefig(FIGURES/(name+".png"),dpi=170,bbox_inches="tight");fig.savefig(FIGURES/(name+".svg"),bbox_inches="tight");plt.close(fig)
    fig,axes=plt.subplots(1,4,figsize=(14,4.2),sharey=True)
    for ax,attack in zip(axes,PROTOCOL["attacks"]):
        labels=[]
        for j,(detector,mode) in enumerate((d,m) for d in PROTOCOL["detectors"] for m in ("attacked","protected")):
            g=next(g for g in groups if (g["attack"],g["detector"],g["mode"])==(attack,detector,mode));p=g["success_rate"]
            ax.bar(j,p,color=colors[detector],alpha=1 if mode=="attacked" else .5)
            ax.errorbar(j,p,yerr=[[max(0,p-g["success_wilson_low"])],[max(0,g["success_wilson_high"]-p)]],color="#222",fmt="none",capsize=4)
            labels.append(("Fixe" if detector=="fixed" else "Adapt.")+"\n"+("Att." if mode=="attacked" else "Prot."))
        ax.set_xticks(range(4),labels);ax.set_title(names[attack]);ax.set_ylim(0,1.05);ax.yaxis.set_major_formatter(PercentFormatter(1));ax.grid(axis="y",alpha=.2)
    axes[0].set_ylabel("Succès empirique des attaques")
    fig.suptitle("30 seeds par barre — IC 95 % Wilson (bootstrap percentile fourni dans les tableaux)",y=1.04)
    save(fig,"m5_success_rates")
    fig,axes=plt.subplots(2,2,figsize=(11,8))
    endpoints={"attack_51":("payment_loss",1,"jeton pédagogique"),"selfish_mining":("adversary_active_share",100,"points de pourcentage"),"block_withholding":("honest_payout",1,"unités de récompense honnête"),"sybil":("victim_height",1,"blocs de hauteur victime")}
    for ax,attack in zip(axes.flat,PROTOCOL["attacks"]):
        metric,factor,unit=endpoints[attack];values=[]
        for detector in PROTOCOL["detectors"]:
            a=grouped[attack,detector,"attacked"];p=grouped[attack,detector,"protected"]
            values.append(np.array([y[metric]-x[metric] for x,y in zip(a,p)])*factor)
        ax.boxplot(values,positions=[0,1],widths=.35,showfliers=False)
        for j,(detector,v) in enumerate(zip(PROTOCOL["detectors"],values)):
            ax.scatter(j+rng_for(attack+detector).uniform(-.1,.1,len(v)),v,color=colors[detector],s=18,alpha=.6)
            d=describe(v.tolist(),f"figure:{attack}:{detector}")
            ax.errorbar(j+.2,d["mean"],yerr=[[max(0,d["mean"]-d["mean_ci_low"])],[max(0,d["mean_ci_high"]-d["mean"])]],fmt="D",color="black",capsize=4,markersize=4)
        ax.axhline(0,color="gray",ls="--");ax.set_xticks([0,1],["Fixe (n=30)","Adaptatif (n=30)"]);ax.set_title(names[attack]);ax.set_ylabel("Protégé − attaqué\n"+unit);ax.grid(axis="y",alpha=.2)
    fig.suptitle("Différences appariées par seed — points individuels, boîte et moyenne + IC bootstrap 95 %",y=1.02)
    save(fig,"m5_paired_effects")
    fig,axes=plt.subplots(2,2,figsize=(11,8))
    for ax,attack in zip(axes.flat,PROTOCOL["attacks"]):
        datasets=[];labels=[]
        for detector in PROTOCOL["detectors"]:
            for mode in ("attacked","protected"):
                vals=[r["latency_ticks"] for r in grouped[attack,detector,mode] if r["latency_ticks"] is not None]
                datasets.append(vals);labels.append(("F" if detector=="fixed" else "A")+"-"+("Att" if mode=="attacked" else "Prot")+f"\nn={len(vals)}, ND={30-len(vals)}")
        ax.boxplot(datasets,showfliers=True);ax.set_xticks(range(1,5),labels);ax.set_title(names[attack]);ax.set_ylabel("Ticks depuis le début d'exposition");ax.grid(axis="y",alpha=.2)
    fig.suptitle("Latence conditionnelle à la détection — ND non imputés ; F=fixed, A=adaptative",y=1.02)
    save(fig,"m5_detection_latency")
    fig,axes=plt.subplots(1,4,figsize=(14,4))
    for ax,attack in zip(axes,PROTOCOL["attacks"]):
        for j,detector in enumerate(PROTOCOL["detectors"]):
            vals=[r["fp"] for r in grouped[attack,detector,"baseline"]]
            counts=[vals.count(k) for k in range(5)]
            ax.bar(np.arange(5)+(j-.5)*.35,counts,.35,label=detector,color=colors[detector])
        ax.set_xticks(range(5));ax.set_ylim(0,31);ax.set_title(names[attack]);ax.set_xlabel("Familles FP par run honnête");ax.grid(axis="y",alpha=.2)
    axes[0].set_ylabel("Nombre de seeds (n=30/détecteur)");axes[-1].legend();save(fig,"m5_honest_false_positives")
    fig,axes=plt.subplots(1,2,figsize=(11,4.5),sharey=True)
    for ax,detector in zip(axes,PROTOCOL["detectors"]):
        vals=[[100*r["adversary_active_share"] for r in grouped["selfish_mining",detector,mode]] for mode in PROTOCOL["modes"]]
        ax.boxplot(vals);ax.set_xticks([1,2,3],["Honnête","Attaqué","Protégé"]);ax.axhline(40,color="#a33",ls="--",label="Hash power adverse : 40 %");ax.set_ylim(0,100);ax.set_title(detector+" — n=30/mode");ax.legend();ax.grid(axis="y",alpha=.2)
    axes[0].set_ylabel("Part de blocs actifs adverses (%)");save(fig,"m5_selfish_distribution")
    fig,axes=plt.subplots(1,4,figsize=(14,4))
    for ax,attack in zip(axes,PROTOCOL["attacks"]):
        vals=[]
        for detector in PROTOCOL["detectors"]:
            vals.append([p["messages"]-a["messages"] for a,p in zip(grouped[attack,detector,"attacked"],grouped[attack,detector,"protected"])])
        ax.boxplot(vals);ax.set_xticks([1,2],["Fixe","Adapt."]);ax.axhline(0,color="gray",ls="--");ax.set_title(names[attack]);ax.grid(axis="y",alpha=.2)
    axes[0].set_ylabel("Messages supplémentaires\nprotégé − attaqué (n=30 paires)");save(fig,"m5_network_costs")
    return matplotlib.__version__

def fmt(value,digits=3):return "non disponible" if value is None else f"{value:.{digits}f}"
def ci(d,scale=1):
    if not d["n"]:return "non disponible"
    if d["mean_ci_low"] is None:return f"{d['mean']*scale:.3f} [IC non disponible, n=1]"
    return f"{d['mean']*scale:.3f} [{d['mean_ci_low']*scale:.3f} ; {d['mean_ci_high']*scale:.3f}]"

def report(rows,records,groups,stats,grouped,pairs,matplotlib_version):
    ref=b.read(b.REFERENCE)
    manifest=[json.loads(line) for line in (b.DEFAULT_OUTPUT/"manifest.jsonl").read_text(encoding="utf-8").splitlines() if line]
    launched=[r for r in manifest if r["status"]=="running"]
    post_errors=[r for r in manifest if r["status"]=="error" and r.get("exit_code")==0]
    engine_errors=[r for r in manifest if r["status"]=="error" and r.get("exit_code") not in (None,0)]
    valid=list(records.values())
    cpu=[r["child_cpu_seconds"] for r in valid if r.get("child_cpu_seconds") is not None]
    rss=[r["child_sampled_peak_rss_bytes"] for r in valid if r.get("child_sampled_peak_rss_bytes") is not None]
    all_summaries=[b.read(b.ROOT/r["summary_path"]) for r in valid if r.get("summary_path")]
    memory=max(r.get("orchestrator_observed_rss_bytes",0) for r in all_summaries)
    disk_bytes=sum(p.stat().st_size for p in b.DEFAULT_OUTPUT.rglob("*") if p.is_file())
    lines=["# Blockchain Security Lab — Benchmark M5 (moteur M4 gelé)","",
        "## Exécution et protocole", "",
        f"**720/720 simulations évaluées valides**, 30 seeds pour chacune des 24 combinaisons ; {len(engine_errors)} échec de processus C++.",
        f"En complément : 4 calibrations honnêtes et 3 essais pilotes, soit **{len(launched)} lancements C++ au total**. La reprise a relu les résultats validés sans nouveau lancement.",
        f"{len(post_errors)} erreurs initiales de résumé Python (`hash` au lieu de `block` dans la trace) ont été corrigées en retraitant les exports existants, sans relancer le C++ ni effacer les erreurs du manifeste.",
        "",f"Seeds finales préenregistrées : {', '.join(map(str,PROTOCOL['evaluation_seeds']))}. Pilote : 9001/9002, avec répétition 9001. Calibration : seed 7 M4 inchangée, référence distincte par attaque. Seed 42 exclue des évaluations.",
        "Les références sont partagées entre les 90 runs adaptatifs de leur configuration, sans ajustement sur les résultats attaqués/protégés. Les deux détecteurs sont réinitialisés entre runs.",
        "Les variations autorisées des JSON M4 sont la seed et le chemin de référence uniquement. Aucun seuil, budget, hashrate, réseau ou mécanisme de défense n'a été modifié.",
        "", "## Preuve de conservation M4", "", f"Commit : `{ref['git_commit']}`. L'empreinte du binaire exécuté est `{ref['binaries'][b.relative(b.BINARY)]}`.",
        f"Les SHA-256 de **{len(ref['files'])} fichiers d'entrée et {len(ref['binaries'])} binaires/DLL** sont dans [m5_reference.json](m5_reference.json). L'intégrité est vérifiée au début, à la reprise et à la fin ; le binaire est aussi revérifié entre simulations.",
        "Le préfixe complet de DEVELOPMENT_LOG antérieur à M5 est protégé par longueur et empreinte ; seule une entrée M5 est ajoutée. Les sources/configurations M4, tests et presets sont restés inchangés, même indépendamment du suivi Git.",
        "Aucune compilation ni relance de CTest : les 44 tests M4 étaient validés et le moteur n'a pas changé. Seuls les tests Python de l'orchestrateur/statistiques ont été exécutés.",
        "", "## Paramètres de référence", "", "| Profil | Nœuds | Hash power | Blocs max | Intervalle moyen (ticks) | Fenêtre (ticks) | Difficulté |", "|---|---:|---|---:|---:|---:|---:|"]
    parameters={}
    for attack in PROTOCOL["attacks"]:
        p=b.read(b.ROOT/f"configs/m4/{attack}_adaptive_protected.json");parameters[attack]=p
        lines.append(f"| {attack} | {len(p['nodes'])} | {' / '.join(str(n['hash_power']) for n in p['nodes'])} | {p['max_blocks']} | {p['mean_block_ticks']} | {p['security']['window_ticks']} | {p['difficulty']} |")
    lines += ["", "Audits : 100 ticks ; horizon : 100000 ticks (Sybil : 3000), limites événements et file : 100000 ; budget PoW : 1000000 essais. EWMA alpha=0,1, k=3, clip=2, drift_cap=2, cooldown=500 ; aucune optimisation.",
        "La probation selfish reste 100 ticks ; chaque expérience isole la mitigation correspondante. Les configurations originales complètes sont archivées dans [m5_parameters.json](m5_parameters.json).", "",
        "## Méthode statistique", "",
        "L'unité indépendante de rééchantillonnage est la **seed**, pas l'alerte, le nœud ou la famille. Les modes et détecteurs d'une même seed sont appariés. Les 720 runs ne sont pas 720 observations indépendantes d'un même effet.",
        "Pour chaque métrique/configuration : n disponible/manquant, moyenne, médiane, écart-type d'échantillon (n−1), min/max, IC 95 % bootstrap percentile des moyennes et médianes, 5000 rééchantillonnages déterministes. Les différences sont formées par seed avant bootstrap.",
        "Les scores principaux sont les scores micro issus des sommes TP/FP/TN/FN M4, avec bootstrap par seed des vecteurs de confusion. Les moyennes des scores par run sont aussi fournies dans la table longue ; elles ne sont pas interchangeables avec les scores micro.",
        "Unité M4 : (run, famille principale), quatre décisions par run. Les alertes d'autres familles restent des FP de classement, même si elles décrivent une anomalie réelle. Baseline n'a pas de positifs : rappel/F1 zéro par convention M4, sans interprétation de performance sur des attaques absentes.",
        "Les latences nulles restent **non détectées/censurées**, jamais remplacées par zéro ou une durée favorable. Les statistiques de latence sont conditionnelles à la détection et leur n est indiqué. La latence part du début d'exposition t=0, pas d'une découverte privée.",
        "Pour les proportions aux frontières 0/30 ou 30/30, le bootstrap peut être dégénéré ; les intervalles Wilson 95 % sont aussi fournis et utilisés dans la figure de succès. Aucun IC ponctuel [1,1] n'est présenté comme preuve de probabilité réelle 1.",
        "Les IC sont conditionnels aux références honnêtes seed 7 gelées et aux paramètres M4. Ils n'intègrent pas l'incertitude d'une nouvelle calibration et ne sont pas corrigés pour des comparaisons multiples.",
        "", "## Tableau synthétique : runs attaqués et protégés", "",
        "Succès : moyenne [IC bootstrap 95 %], n=30 par ligne. P/R/F1 : micro sur 30 clusters, pas sur des alertes indépendantes.","",
        "| Attaque | Détecteur | Mode | Succès | P | R | F1 | TP / FP / FN | Latence moyenne (ticks), n détectés |",
        "|---|---|---|---|---:|---:|---:|---|---|"]
    for g in groups:
        if g["mode"]=="baseline":continue
        key=g["attack"],g["detector"],g["mode"]
        lines.append(f"| {g['attack']} | {g['detector']} | {g['mode']} | {ci(stats[key,'attack_success'])} | {g['micro_precision']:.3f} | {g['micro_recall']:.3f} | {g['micro_f1']:.3f} | {g['tp']} / {g['fp']} / {g['fn']} | {fmt(g['latency_ticks_mean'],1)}, n={g['detected_n']} |")
    s_att=stats[("selfish_mining","fixed","attacked"),"adversary_active_share"]["mean"]
    s_prot=stats[("selfish_mining","fixed","protected"),"adversary_active_share"]["mean"]
    worsened=sum(p["adversary_active_share"]>a["adversary_active_share"] for a,p in zip(grouped["selfish_mining","fixed","attacked"],grouped["selfish_mining","fixed","protected"]))
    lines += ["", "### Lecture des résultats", "",
        "- **51 %** : 30/30 doubles dépenses réussies, avec ou sans protection, pour les deux détecteurs. Le fixe détecte 30/30 expositions, l'adaptatif 28/30 ; ses deux non-détections sont conservées. La profondeur maximale de réorganisation à la cible est en moyenne 2,1 blocs, intervalle observé 2–3. Les nouvelles confirmations n'agissent pas sur un paiement déjà accepté.",
        f"- **Selfish** : 10/30 runs dépassent le hash power de 40 % sans défense, contre 4/30 protégés. La part active moyenne passe de {100*s_att:.2f} % à {100*s_prot:.2f} %. L'attaque non protégée n'est donc **pas rentable en moyenne au sens de ce proxy** sur ces seeds, contrairement à ce que pouvait suggérer la démonstration seed 42. La défense augmente malgré tout la part adverse sur {worsened}/30 paires : son bénéfice moyen n'est pas universel.",
        "- **Pool** : 39,57 solutions complètes retenues en moyenne (31–46 observées). Récompense pool moyenne 98,63 en baseline, 60,43 attaquée et 60,43 protégée : l'audit redistribue mais ne restaure pas les blocs retenus. Le fixe réagit plus tôt et réduit davantage le payout adverse ; l'adaptatif réduit les FP de classement sur l'attaque, au prix d'une latence plus longue. Aucune share honnête non créditée dans les 30 cas protégés par détecteur ; ce zéro observé ne prouve pas l'absence de risque.",
        "- **Sybil** : 30/30 isolements réussis sans défense, 0/30 après protection. Le délai depuis le premier avertissement reste 100 ticks ; le rattrapage génère aussi des alertes de publication faussement attribuées à une autre famille.",
        f"Aucun run évalué ne s'est arrêté par horizon/file/événements avec données en transit : {sum(g['censored_by_budget_n'] for g in groups)}/720. Cela ne lève pas la censure des non-détections à la fin des runs."]
    lines += ["", "## Baselines honnêtes : faux positifs", "", "| Profil | Détecteur | Familles FP cumulées / 120 unités | Runs avec au moins une FP / 30 |", "|---|---|---:|---:|"]
    for g in groups:
        if g["mode"]=="baseline":
            count=sum(r["fp"]>0 for r in grouped[g["attack"],g["detector"],"baseline"])
            lines.append(f"| {g['attack']} | {g['detector']} | {g['fp']} | {count} |")
    lines += ["", "## Effets appariés par attaque", "", "Les valeurs ci-dessous sont protégé − attaqué, sauf indication. Chaque paire partage seed et paramètres de référence. Les délais/défenses peuvent ensuite faire diverger les trajectoires."]
    endpoints={"attack_51":["payment_loss","max_reorg_depth","accepted_at","payment_deferrals"],"selfish_mining":["adversary_active_share","active_share_minus_hash_share","target_stale_ratio","honest_recipient_delays","honest_producer_payload_delays"],"block_withholding":["pool_reward","adversary_payout","honest_payout","honest_shares_uncredited"],"sybil":["victim_height","victim_known_blocks","recovery_after_reconnect_ticks","sync_messages","messages"]}
    for attack in PROTOCOL["attacks"]:
        lines += ["",f"### {attack}","", "| Détecteur | Mesure | Différence moyenne [IC bootstrap 95 %] | Paires complètes |", "|---|---|---|---:|"]
        for detector in PROTOCOL["detectors"]:
            for metric in endpoints[attack]:
                d=next(p for p in pairs if p["attack"]==attack and p["detector"]==detector and p["comparison"]=="protected_minus_attacked" and p["metric"]==metric)
                lines.append(f"| {detector} | {metric} | {ci(d)} | {d['n']} |")
        if attack=="attack_51":lines += ["", "Paiements annulés = au plus un jeton pédagogique par run réussi. `accepted_at` compare uniquement les paires où les deux marchands acceptent ; les non-acceptations ne valent pas délai nul. Le nombre exact de confirmations à l'acceptation est **non disponible** : seul le tick d'acceptation est exporté, sans événement séquencé ni seuil appliqué à cet instant. La règle initiale de 2 confirmations est connue. Un coût de fausse suspension sur un run entièrement honnête protégé n'est pas isolable : le protocole M4 ne contient pas ce quatrième mode."]
        if attack=="selfish_mining":lines += ["", "La part active et son écart au hash power de 40 % ne sont pas une valeur monétaire. Les stale sont ceux connectés hors chaîne dans la vue cible, jamais les blocs privés non publiés. Les reports aux destinataires honnêtes et aux contenus de producteurs honnêtes sont mesurables ; l'effet exact sur des **relais émetteurs honnêtes** est non disponible, car la trace de probation n'exporte pas l'émetteur."]
        if attack=="block_withholding":
            lines += ["", "| Détecteur | Mode | Récompense pool moyenne | Payout adverse moyen | Payout honnête moyen | Runs avec shares honnêtes non créditées |", "|---|---|---:|---:|---:|---:|"]
            for detector in PROTOCOL["detectors"]:
                for mode in PROTOCOL["modes"]:
                    key=attack,detector,mode;g=next(g for g in groups if (g["attack"],g["detector"],g["mode"])==key)
                    lines.append(f"| {detector} | {mode} | {fmt(g['pool_reward_mean'])} | {fmt(g['adversary_payout_mean'])} | {fmt(g['honest_payout_mean'])} | {g['honest_sanction_runs']}/30 |")
            lines += ["", "Les shares observées incluent celles non créditées. La suspension change le partage des récompenses et ne récupère aucune solution complète retenue. Les pertes honnêtes contre baseline se trouvent dans les différences `attacked_minus_baseline` ; elles intègrent les conséquences réelles sur la chaîne, sans modèle monétaire externe."]
        if attack=="sybil":
            lines += ["", "| Détecteur protégé | Récupération moyenne [IC 95 %], ticks | Médiane | Min–max | n |", "|---|---|---:|---|---:|"]
            for detector in PROTOCOL["detectors"]:
                d=stats[(attack,detector,"protected"),"recovery_after_reconnect_ticks"]
                lines.append(f"| {detector} | {ci(d)} | {fmt(d['median'],1)} | {fmt(d['min'],0)}–{fmt(d['max'],0)} | {d['n']} |")
            lines += ["", "Le délai de récupération rapporté est **première adoption valide après la première reconnexion**, moins son tick. Il ne signifie pas convergence complète. Une différence appariée avec attacked n'est pas calculable, puisqu'aucune reconnexion n'a lieu dans ce mode. Le total de blocs connus finaux de la victime est observable ; le nombre exact de blocs uniques récupérés **uniquement** par rattrapage est non disponible, faute de provenance des messages. Les messages sync sont des envois, pas des blocs uniques."]
    lines += ["", "## Coûts et ressources observés", "", "Les coûts simulés restent séparés du temps CPU réel. Tables longues et différences appariées incluent messages, événements, évaluations de caractéristiques, échantillons conservés, probation, crédits et reconnexions.","",
        f"CPU total C++ mesuré par GetProcessTimes, calibrations/pilote inclus : **{sum(cpu):.3f} s**, n={len(cpu)} processus. Le wall time est mesuré séparément dans chaque résumé.",
        f"Maximum RSS C++ **échantillonné** à environ 10 ms : **{max(rss)/2**20:.2f} MiB** ; ce n'est pas une garantie de pic exact. RSS de l'orchestrateur observée entre runs : max **{memory/2**20:.2f} MiB**. Un seul processus de simulation à la fois.",
        f"Données de campagne sur disque au moment de l'analyse : **{disk_bytes/2**20:.2f} MiB**. Espace initial disponible : {ref['initial_disk_free_bytes']/2**30:.2f} GiB. Aucun fichier de résultat n'a été supprimé.",
        "Le coût des seuils est mesuré en opérations logiques, pas en pourcentage CPU du détecteur : le moteur M4 ne profile pas ses sous-modules. Le RSS/temps de l'analyse bootstrap ne doit pas être confondu avec le coût des défenses.",
        "Les temps réels concernent des processus courts, exécutés dans l'ordre déclaré, sans randomisation temporelle ni machine dédiée. Ils peuvent dépendre du cache et de l'activité du PC (dont la préparation locale des outils graphiques). Les comparaisons CPU/wall sont exploratoires ; les métriques simulées restent reproductibles.",
        "", "| Attaque | Détecteur | Coût protégé − attaqué | Moyenne [IC bootstrap 95 %] | Paires |", "|---|---|---|---|---:|"]
    for attack in PROTOCOL["attacks"]:
        for detector in PROTOCOL["detectors"]:
            for metric in ("messages","events","child_cpu_seconds"):
                d=next(p for p in pairs if p["attack"]==attack and p["detector"]==detector and p["comparison"]=="protected_minus_attacked" and p["metric"]==metric)
                lines.append(f"| {attack} | {detector} | {metric} | {ci(d)} | {d['n']} |")
    lines += [
        "", "## Résultats inattendus et limites scientifiques", "",
        "- Les FP des baselines bruyantes et les anomalies d'autres familles sont conservés. L'adaptatif n'est pas supposé supérieur sur toutes les métriques ; sa latence peut être plus longue.",
        "- Les gains apparents peuvent dépendre de la seed, du nombre fini de blocs, du départage lexicographique et des délais M4. Une diminution moyenne de part active ne prouve pas élimination du selfish mining.",
        "- L'absence de message avertisseur avant une chaîne privée limite la politique marchande ; une alerte après acceptation ne peut sauver le paiement passé.",
        "- Un pool peut rester perdant après audit ; priver de crédit des shares n'est pas prouver une fraude. Les fausses sanctions sont mesurées pour les membres honnêtes dans les runs protégés.",
        "- La défense Sybil suppose un annuaire et des groupes administratifs fiables. Les groupes distincts ne prouvent pas l'honnêteté. Les identités n'ajoutent aucun hash power.",
        "- Une seule référence honnête seed 7 par profil : incertitude de calibration non explorée. Pas de réoptimisation des paramètres après lecture des résultats.",
        "- Les latences des non-détections sont censurées ; une moyenne conditionnelle courte peut être trompeuse sans le nombre de FN. Les horizons d'arrêt sont conservés.",
        "- Les erreurs Python de résumé ont été réparées sur les mêmes fichiers bruts, sans filtrage de seeds ni sélection de résultats favorables. Les empreintes de réparation sont journalisées.",
        "- Portée : modèle PoW pédagogique, pas Bitcoin/Ethereum réel, pas d'extrapolation au PoS, pas de généralisation au-delà des paramètres testés.",
        "", "## Artefacts", "",
        "- [Toutes les 720 lignes légères](tables/m5_runs.csv)","- [24 campagnes : taux et scores](tables/m5_groups.csv)","- [Statistiques complètes, n, dispersion et IC](tables/m5_statistics.csv)","- [Différences appariées et comparaison adaptatif/fixe](tables/m5_paired.csv)",
        "- [Protocole préenregistré](m5_protocol.json) ; [paramètres M4](m5_parameters.json) ; [preuves d'intégrité](m5_integrity.json)",
        "", "### Graphiques", ""]
    for stem,title in (("m5_success_rates","Succès et incertitude"),("m5_paired_effects","Effets appariés"),("m5_detection_latency","Distribution des latences et non-détections"),("m5_honest_false_positives","Faux positifs honnêtes"),("m5_selfish_distribution","Distribution de la part active selfish"),("m5_network_costs","Coût réseau")):
        lines += [f"![{title}](figures/{stem}.png)",f"[Version vectorielle SVG](figures/{stem}.svg)",""]
    (OUT/"M5_BENCHMARK_REPORT.md").write_text("\n".join(lines)+"\n",encoding="utf-8")
    b.write(OUT/"m5_parameters.json",{p.name:b.read(p) for p in sorted((b.ROOT/"configs/m4").glob("*.json"))})
    b.write(OUT/"m5_integrity.json",{"verified_utc":b.now(),"reference_manifest_sha256":b.digest(b.REFERENCE),"m4_unchanged":True,
        "git_commit":ref["git_commit"],"input_files_verified":len(ref["files"]),"binaries_verified":len(ref["binaries"]),
        "evaluation_valid":720,"calibrations_valid":4,"pilot_valid":3,"cpp_launches":len(launched),"cpp_failures":len(engine_errors),
        "python_extraction_errors_repaired_without_cpp":len(post_errors),"source_csv_sha256":b.digest(TABLES/"m5_runs.csv"),
        "analysis_sha256":b.digest(Path(__file__)),"environment":{"numpy":np.__version__,"matplotlib":matplotlib_version,"python":sys.version},
        "resources":{"cpp_cpu_seconds":sum(cpu),"sampled_cpp_peak_rss_bytes":max(rss),"observed_orchestrator_rss_bytes":memory,"raw_campaign_bytes":disk_bytes}})

def main():
    rows,records=load_summaries();groups,stats,grouped,pairs=aggregate(rows)
    version=figures(groups,stats,grouped)
    report(rows,records,groups,stats,grouped,pairs,version)
    b.verify_reference()
    print(json.dumps({"evaluations":len(rows),"cohorts":len(groups),"valid_per_cohort":30,"statistics_rows":len(stats),"paired_rows":len(pairs),"m4_unchanged":True}))

if __name__=="__main__":main()
