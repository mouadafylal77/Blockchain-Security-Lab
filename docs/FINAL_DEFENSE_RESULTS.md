# Renforcement M4 : résultats ciblés et limites

Politique implémentée : `security.hardened_security=true`, export `policy_revision=hardened_m4_v1`. Exemples neufs sous `configs/final_defenses/`. Activation explicite : les anciennes configurations et le terminal restent inchangés. Aucun commit ni push.

Protocole défini avant les modifications : [FINAL_DEFENSE_PROTOCOL.md](FINAL_DEFENSE_PROTOCOL.md). Seeds **42 et 43**, présentées dans cet ordre ci-dessous. Huit paires attaqué/protégé, huit paires de contrôles honnêtes, sans modification des ressources ou de la stratégie entre les variantes. Vérification des JSON exportés : seule `security.mode` diffère dans chaque paire attaquée ; mêmes découvertes totales. Horizons : 5 000 ticks (51 % et selfish), 40 000 (pool), 3 000 (Sybil), max_blocks 10 000 et max_events 1 000 000 non atteints. Tous les runs terminent à l'horizon prévu, sans drainage artificiellement prolongé pour une variante. Les résultats sont des observations à horizon fini, pas une estimation générale.

| Attaque | Dommage sans défense (42 / 43) | Dommage avec défense (42 / 43) | Coût honnête |
|---|---|---|---|
| Double dépense, 85 % de hash power | 1 / 1 paiement accepté puis annulé | 1 / 1 paiement accepté puis annulé | Paiements honnêtes retardés de 127 / 102 ticks, tous deux acceptés |
| Selfish, 40 % | Part adverse active : 47,16 % / 32,93 % | 28,29 % / 10,32 % | 0 contenu honnête directement retardé ; hauteur totale réduite de 24 / 12 blocs dans les runs attaqués |
| Withholding, 40 % | Payout adverse distribuable : 42,6241 / 45,5223 unités ; perte de pool : 93 / 71 unités | Payout distribuable : 8,2968 / 9,1136 ; perte de pool inchangée : 93 / 71 | 0 crédit honnête séquestré dans ces contrôles |
| Sybil | Connexions exclusivement adverses pendant au moins 3 000 ticks ; aucun bloc adopté | Lien indépendant à t=0 ; premier bloc adopté à 48 / 304 | 2 connexions supplémentaires ; +96 / +100 messages dans le contrôle honnête |

## Ce que les mesures permettent de conclure

51 % : acceptation attaquée à 74 / 279 sans défense, à 727 / 814 avec plancher de six confirmations ; une annulation dans chaque variante. Contrôle honnête : 19 / 162 contre 146 / 264. Aucune action sur un paiement déjà accepté ni sur un nœud non marchand : ces réactions sans bénéfice ont été supprimées dans la politique renforcée. Le plancher est effectif dès t=0, avant toute alerte. Il retarde l'acceptation sans empêcher cette majorité. Les seuils existants et le contexte public ne donnent pas de signal persistant exploitable suffisamment tôt dans ces deux exemples. Il serait faux de présenter l'attente comme une protection démontrée.

Test de causalité séparé, pas un taux de succès : trois mineurs 10/45/45 et liens à 40 ticks, audit public événementiel et `proactive_signals=true`. Première intervention à t=65, occasion de base à t=206, acceptation renforcée à t=512, trois actions avant acceptation. Le détecteur ne reçoit ni branche privée, ni rôle adverse, ni puissance cachée. Un cas tardif et majoritaire reste présent dans les contrôles. Paramètres de ce test explicités dans `tests/test_final_defenses.cpp`.

Selfish : 192 / 115 messages de producteur adverse retardés de 100 ticks ; parmi eux 94 / 55 ont un relais honnête. Aucun message de producteur honnête directement retardé dans ces cas. La correction évite de punir le contenu honnête parce qu'il traverse un relais suspect. Chaîne du témoin : hauteur 176 -> 152 et 167 -> 155 ; blocs honnêtes actifs 93 -> 109 et 112 -> 139. Dans les contrôles entièrement honnêtes, aucun report et aucune modification de chaîne. Les branches privées et messages restant en attente à l'horizon ne sont pas ignorés : l'issue M3 protégée est **inconclusive** dans ces deux runs. La réduction de part active n'est pas un taux d'échec définitif de l'attaque.

Pool : une seule activation par run ; 1 047 / 1 131 crédits adverses encore en séquestre à l'horizon, aucun crédit honnête retenu. Le modèle de déficit exige des observations suffisantes et une borne de queue sous le modèle nominal, pas seulement un z-score configurable arbitrairement sensible. Aucun bloc perdu n'est restauré : récompense de pool attaquée/protégée 110 / 111, contre référence honnête 203 / 182. Les 95 / 71 solutions retenues ne sont pas assimilées automatiquement à une perte économique identique : perte contrôlée 93 / 71. Le payout ci-dessus est immédiatement distribuable selon les crédits M3 à l'horizon, **pas une économie définitive** : les crédits séquestrés restent restituables. Un test vérifie effectivement la restitution dans la comptabilité servant au payout, le coût crédit*ticks, l'expiration sans nouvelles données et l'exonération sur fenêtre rassurante. Les deux seeds honnêtes sans sanction ne donnent aucune garantie de taux nul de faux positifs.

Sybil : ne reconfigurer que le client public désigné (`target`), au lieu de tous ses voisins, réduit les coûts sans coopération forcée des identités adverses. Dans les runs attaqués, messages 144 -> 264 et 150 -> 275 ; dans les contrôles honnêtes, 192 -> 288 et 200 -> 300 (+50 %). Deux liens ajoutés et un lien redondant du même groupe écarté dans la vue attaquée. Cette charge réseau est réelle et dépend de la petite topologie choisie. Le bootstrap peut être retardé par configuration. Si l'annuaire public ne contient que les identités Sybil, le test négatif conserve une victime isolée et une attaque réussie : aucun pair indépendant n'est choisi par connaissance secrète. Les métriques distinguent première connexion indépendante, première adoption et convergence non prouvée.

## Vérifications exécutées

- Avant modification : `./scripts/preserve-m5.ps1 -Create`, puis `./scripts/test-ucrt64.ps1` : **67/67**, 30,92 s.
- Deux essais de rebuild via le script ont rencontré `cannot open output file blockchain_sim.exe: Permission denied`. Aucun processus utilisateur n'a été arrêté et aucun exécutable existant n'a été supprimé.
- Même preset UCRT64, répertoire de sortie local distinct :

```powershell
# PATH de la session : UCRT64/bin et outils CMake/Ninja Visual Studio existants.
cmake --preset ucrt64 '-DCMAKE_MAKE_PROGRAM=C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe' '-DCMAKE_RUNTIME_OUTPUT_DIRECTORY=C:/Users/LENOVO/OneDrive/Documents/ChatGPT/Blockchain Security Lab/build/ucrt64/final-bin'
cmake --build --preset ucrt64 --parallel 2
ctest --preset ucrt64 -R '^final_defense_' --output-on-failure
ctest --preset ucrt64 --output-on-failure
./build/ucrt64/final-bin/blockchain_sim.exe --demo
```

CMake/Ninja, GCC 14.2.0 UCRT64, C++17/OpenSSL. Premier lot ciblé : **5/5**, 8,72 s. Dernière suite après limitation des interventions au client et suppression des réactions post-acceptation : **72/72, zéro échec, 37,43 s**. Démonstration du nouveau binaire : `chain_valid=true`. Aucun warning/erreur dans le dernier log de build. Les cinq nouveaux tests sont `final_defense_policy`, `final_defense_merchant`, `final_defense_selfish`, `final_defense_pool`, `final_defense_sybil` ; le premier couvre aussi paramètres invalides, persistance, causalité publique, séquestre et restitution.

Logs locaux : `build/final-defense-configure.log`, `build/final-defense-build-final.log`, `build/final-defense-ctest-final.log`. Exports réexécutables de tests : `build/ucrt64/final-defenses/<attaque>_<seed>_<mode>/results.json`. Le binaire courant testé est **`build/ucrt64/final-bin/blockchain_sim.exe`** ; l'exécutable bloqué de l'ancien chemin n'est pas annoncé comme mis à jour. Les presets et le script suivis par Git sont inchangés.

Préservation : 108 fichiers préexistants vérifiés par SHA-256 (core, attaques, terminal, configurations, analysis, exo4, réseau/RNG/file d'événements), aucune divergence. `scripts/preserve-m5.ps1` : 142 fichiers d'archive et 2 914 fichiers historiques vérifiés. Aucun des 720 runs M5 n'a été relancé pour remplacement.

## Fichiers de ce lot

Modifiés : `CMakeLists.txt` ; `src/detection/detector.hpp`, `detector.cpp`, `config_io.cpp`, `profile.cpp` ; `src/mitigation/policy.hpp`, `policy.cpp` ; `src/sim/simulator.hpp`, `simulator.cpp`, `security.cpp`, `mining_pool.hpp` ; `src/metrics/security_export.cpp`, `security_study.cpp` ; `README.md`, `docs/SIMULATION_MODEL.md`, `docs/DEVELOPMENT_LOG.md`.

Créés : `tests/test_final_defenses.cpp` ; `configs/final_defenses/{attack_51,selfish_mining,block_withholding,sybil}.json` ; `docs/FINAL_DEFENSE_PROTOCOL.md` et ce rapport. Les modifications non commitées antérieures sont conservées.

Limites restantes : majorité PoW non neutralisée ; plancher marchand à justifier économiquement plutôt qu'une constante magique ; probation expérimentale fondée sur identité/horodatage fiables du producteur et calibration réseau pertinente ; séquestre réversible, pas réparation du hash power détourné ; quotas de groupes publics et disponibilité de bootstrap supposés ; délais des nouveaux liens d'un tick hérités du modèle. Les bornes de queue nominales et les fenêtres recouvrantes ne constituent pas une calibration empirique multi-seeds. Les références adaptatives liées doivent correspondre au nouvel extracteur ; la topologie initiale ne décrit pas forcément la distribution après mitigation. Aucun nouveau résultat de calibration adaptative générale n'est revendiqué. Aucune protection garantie ni nouvelle campagne scientifique complète.
