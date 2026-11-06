# Journal de développement

## 2026-11-04 — M0 : audit et alignement

Sources lues : `projet.md`, `AGENTS.md`, `PLAN.md`, puis `exo4.cpp`.
L'énoncé fourni est présent et contient les objectifs et livrables ; sa fidélité
à un document externe du professeur ne peut pas être vérifiée depuis le dépôt.

### Exigences traçables

| Exigence de projet.md | Réponse prévue dans PLAN.md | État après ce travail |
|---|---|---|
| Blockchain PoW/PoS issue de l'Atelier 1 | M1 noyau, M2 simulateur ; PoS pédagogique facultatif | PoW minimal seulement ; portée PoS à clarifier |
| 51 %, double dépense et réorganisation | M3 | Différé |
| Selfish mining avec publication stratégique | M3 | Différé |
| Block withholding dans une pool | M3 | Différé |
| Sybil dans un réseau P2P simulé | M2 réseau, M3 attaque | Différé |
| Contribution adaptative sur métriques réseau | M4 détecteur fixe puis adaptatif | Différé |
| Détection par règles ou ML, contre-mesures adaptées | M4, règles d'abord | Conforme : ML facultatif selon l'énoncé |
| Comparaison référence/attaquée/protégée : succès, précision, rappel, délai, pertes, surcoût | M4–M6 | Différé ; unités et vérité terrain à définir |
| Expériences reproductibles : nœuds, hash power, stratégies, réseau ; comparaison à des références | M2, M4–M6 | Déterminisme du noyau uniquement |
| Plateforme, sources, scénarios, tableaux/graphiques, rapport et limites | M1–M7 | Sources initiales et limites documentées |

### Écarts et points non vérifiables

1. Le **premier jalon est M0**, documentaire. Critères : checklist traçable des
   fonctions obligatoires et dépendances initiales, sans génération massive.
   La section 6 demande aussi le plus petit M1 exécutable. L'utilisateur a
   explicitement retenu **M0 + noyau minimal M1** dans cet échange.
2. L'énoncé parle d'une blockchain « PoW/PoS » et d'attaques relatives au consensus
   utilisé ; il ne précise pas si les deux consensus doivent recevoir toutes les
   attaques. Le plan privilégie PoW et rend PoS pédagogique facultatif. C'est un
   risque de portée à clarifier, pas une preuve que PoS est dispensé par l'énoncé.
3. L'énoncé emploie « blocs orphelins ». Le plan distingue stale valide hors chaîne
   et parent inconnu. Les métriques devront expliciter ces définitions lors de M2/M4.
4. Le statut du plan « pas encore validé contre projet.md » est historique : le
   présent audit aligne les objectifs, sans résoudre l'ambiguïté PoS. Aucune autre
   contradiction obligatoire identifiée. Le choix règles plutôt que ML et le
   report du web sont compatibles avec les exigences fournies.
5. Délais, barème, plages de paramètres imposées, statistiques obligatoires,
   format du rapport et interface obligatoire ne figurent pas dans `projet.md`.
   Ils ne peuvent pas être vérifiés et aucune valeur n'est inventée.
6. Le code Atelier 1 mélange affichage, temps réel, PoW et PoS ; il ne fournit
   ni simulation P2P, ni attaques, ni validation stricte de tous les champs du
   genesis. Ses concepts sont réécrits en modules, son fichier reste intact.

### Dépendances et environnement initial

GCC UCRT64 14.2.0-2 et OpenSSL UCRT64 3.4.0-1 sont installés dans `C:/msys64/ucrt64`.
Les paquets UCRT64 CMake et Ninja sont absents (vérification `pacman -Q`).
CMake, Ninja et CTest sont disponibles dans Visual Studio 2022 BuildTools et
servent d'orchestrateurs natifs, avec GCC/OpenSSL UCRT64 explicitement choisis.
Le premier démarrage de bash a été bloqué par le bac à sable Windows ; la
consultation en lecture des versions MSYS2 a ensuite été autorisée et exécutée.
Aucune installation de paquet n'a été effectuée. Disque disponible observé :
environ 35 Go décimaux ; compilation limitée à deux tâches.

## 2026-11-04 — Noyau minimal M1

Implémenté : bibliothèque C++17 `src/core`, SHA-256 OpenSSL, Merkle root,
genesis fixe, minage borné, validation d'intégrité et de continuité d'une chaîne,
CLI minimale, presets CMake/Ninja UCRT64, script PowerShell, tests CTest.
Tests sans framework externe ; choix du parseur JSON différé tant qu'il n'est
pas utilisé. Les hypothèses figurent dans `SIMULATION_MODEL.md`.

Critères d'acceptation M1 visés : compilation C++17, vecteur SHA-256 connu,
chaîne valide, parent invalide malgré PoW correcte, blocs/transactions altérés,
genesis non conforme, limites PoW et minage déterministe ; démonstration CLI
documentée et erreurs non nulles. Sélection de branches non applicable à cette
chaîne linéaire, réservée à M2.

### Validation réellement exécutée

Commande d'entrée PowerShell : `./scripts/test-ucrt64.ps1`.
Premier lancement : échec de lecture du preset, car `jobs` avait été placé
directement dans le test preset. Correction vers `execution.jobs`, puis relance
complète du script : **code de sortie 0**.

Les exécutables employés sont ceux de :

```text
CMake/CTest : C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/
Ninja : C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe
GCC : C:/msys64/ucrt64/bin/g++.exe
Crypto : C:/msys64/ucrt64/lib/libcrypto.dll.a
```

Versions exécutées : CMake/CTest `3.31.6-msvc6`, Ninja `1.12.1`, GCC `14.2.0`,
OpenSSL détecté par CMake `3.4.0`. Le suffixe CMake `msvc6` décrit sa distribution,
pas le compilateur du projet : le cache sélectionne GCC et les règles Ninja
contiennent `-std=c++17 -Wall -Wextra -Wpedantic` pour le noyau.

Commandes lancées par le script (outils ci-dessus ajoutés au PATH de session) :

```powershell
cmake --preset ucrt64 "-DCMAKE_MAKE_PROGRAM=C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe"
cmake --build --preset ucrt64 --parallel 2
ctest --preset ucrt64 --output-on-failure
./build/ucrt64/blockchain_sim.exe --demo
```

Résultats : configuration et génération réussies ; huit étapes Ninja réalisées,
sans avertissement de compilation observé ; **13/13 tests réussis, 0 échec**,
temps total CTest `0,77 s`. Neuf tests noyau : SHA-256, Merkle, chaîne valide,
parent invalide, altérations, genesis, PoW, limites, déterminisme. Quatre tests
CLI : démo, aide, argument inconnu, argument supplémentaire.
La démo a retourné 0 et affiché `chain_valid=true`, nonces `334` et `111` pour
les deux blocs hors genesis. Ce sont des sorties du programme, pas des mesures
d'une campagne expérimentale.

Vérifications finales exécutées : `git diff --check` sans erreur (avertissement
Git de conversion LF/CRLF sur le fichier Agents.md préexistant), cache CMake et
règles Ninja inspectés, `git check-ignore build/ucrt64/blockchain_sim.exe`
confirme que le binaire est ignoré. `git status --short` confirme l'absence de
mise en index des nouveaux fichiers ; la modification préalable de `Agents.md`
n'a pas été touchée. Aucun commit exécuté.

SHA-256 de `exo4.cpp`, identique avant et après :

```text
DD167B553F5CFB36D0FA0DCD711E2F6B79B915B4434CBAC3D37EEAA6EC76EAD2
```

### Fichiers créés (15)

```text
.gitignore
CMakeLists.txt
CMakePresets.json
README.md
src/main.cpp
src/core/sha256.hpp
src/core/sha256.cpp
src/core/block.hpp
src/core/block.cpp
src/core/blockchain.hpp
src/core/blockchain.cpp
tests/test_blockchain.cpp
scripts/test-ucrt64.ps1
docs/SIMULATION_MODEL.md
docs/DEVELOPMENT_LOG.md
```

Restant : JSON/exports et simulation multi-nœuds en M2, puis attaques et mesures.
Aucun backend, frontend, autre jalon ou commit Git dans cette tâche.

## 2026-11-04 — M2 : simulation PoW multi-nœuds

### Alignement et critères

Documents relus : `AGENTS.md`, `PLAN.md`, `projet.md`, modèle, historique, noyau
et tests M1. Aucune contradiction bloquante entre la demande et M2 : le plan
demande RNG, file événementielle, temps simulé, arbres locaux, hash power,
propagation, forks et convergence reproductible. L'entrée JSON et les premières
mesures demandées sont l'incrément nécessaire pour rendre ce M2 exécutable ;
le collecteur complet et les métriques de sécurité M4 restent différés.
Le nom employé dans la documentation actuelle et la CLI est **Blockchain Security Lab**.
Les sources académiques et l'historique M1 ne sont pas renommés.

Acceptation : événements ordonnés même à date égale, exécution bornée, mineurs
distincts des identités réseau, vues indépendantes, propagation et doublons,
attente de parents puis rattachement, sélection par travail et égalité déterministe,
réorganisations mesurables, JSON validé et résultats reproductibles ; conserver
les 13 tests M1 et démontrer un réseau avec forks/réorganisation réelle.

### Implémentation

`BlockTree` est un nouveau module ; aucun changement de l'API ni de l'implémentation
des six fichiers du noyau M1. Les nœuds ont leurs propres copies de blocs et arbres.
Le registre commun sert uniquement aux contenus immuables et origines. Les mineurs
honêtes utilisent des processus exponentiels à taux constants, la tête courante
au moment de la découverte et une PoW M1 vérifiable à difficulté commune.
Les choix et biais de discrétisation, de difficulté et de départage sont détaillés
dans `SIMULATION_MODEL.md`, avec le schéma JSON et toutes les bornes.

Choix JSON : en-tête unique nlohmann/json **v3.11.3**, avec licence MIT et provenance
dans `third_party/nlohmann/README.md`. Aucun gestionnaire ni téléchargement CMake.
Téléchargement autorisé après échec DNS dans le bac à sable ; empreinte du fichier
vérifiée contre la publication officielle :

```text
9bea4c8066ef4a1c206b2be5a36302f8926f7fdc6087af5d20b417d0cf103ea6
```

La CLI délègue à `src/cli.cpp` et conserve la démo M1 sans argument/avec `--demo`.
Le mode M2 exige `--config FILE --output DIRECTORY`, exporte `results.json`,
renvoie 1 sur erreur de configuration/minage/capacité/écriture, 2 pour arguments
incorrects. Aucun résultat d'attaque ou récompense financière n'est fabriqué.

### Fichiers créés (22) et modifiés (6)

```text
Créés :
  src/cli.hpp
  src/cli.cpp
  src/core/block_tree.hpp
  src/core/block_tree.cpp
  src/sim/event.hpp
  src/sim/event_queue.hpp
  src/sim/rng.hpp
  src/sim/config.hpp
  src/sim/config.cpp
  src/sim/node.hpp
  src/sim/network.hpp
  src/sim/network.cpp
  src/sim/simulator.hpp
  src/sim/simulator.cpp
  src/metrics/json_exporter.hpp
  src/metrics/json_exporter.cpp
  tests/test_simulation.cpp
  configs/baseline.json
  configs/fork_reorg.json
  third_party/nlohmann/json.hpp
  third_party/nlohmann/LICENSE.MIT
  third_party/nlohmann/README.md
Modifiés :
  CMakeLists.txt          # version 0.2.0, bibliothèques et nouveaux tests
  src/main.cpp            # délégation CLI minimale
  .gitignore              # résultats bruts locaux exclus
  README.md               # nom officiel, commandes M2
  docs/SIMULATION_MODEL.md # modèle et schémas M2
  docs/DEVELOPMENT_LOG.md  # cette entrée
```

Aucun fichier/dossier vide ajouté. Les sorties dans `results/raw/` et `build/`
sont locales, ignorées par Git. Presets et script PowerShell existants inchangés.
Les fichiers de référence, `PLAN.md`, `AGENTS.md` et `projet.md` sont préservés.

### Commandes et résultats réellement exécutés

Deux passages complets du script `./scripts/test-ucrt64.ps1` :

1. Configuration CMake et compilation Ninja (17 étapes) réussies ; **29/29 tests**
   réussis en **1,75 s**. Démo M1 : `chain_valid=true`, nonces `334` et `111`.
2. Après ajout des tests réseau désordonné/hash power, garde sur les taux numériques
   et métadonnées de construction : configuration et compilation incrémentale
   (6 étapes) réussies ; **31/31 tests**, **0 échec**, en **2,19 s**.
   Aucun avertissement de compilation observé. Démo M1 à nouveau valide.

Pas d'erreur de compilation ou de test constatée pendant M2.

Outils employés : mêmes CMake/CTest 3.31.6-msvc6 et Ninja 1.12.1 fournis par
Visual Studio BuildTools que M1, **GCC UCRT64 14.2.0**, **OpenSSL UCRT64 3.4.0**.
Le script a réellement lancé (chemins complets inchangés, consignés plus haut) :

```powershell
cmake --preset ucrt64 "-DCMAKE_MAKE_PROGRAM=C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe"
cmake --build --preset ucrt64 --parallel 2
ctest --preset ucrt64 --output-on-failure
./build/ucrt64/blockchain_sim.exe --demo
```

Les **13 tests M1** sont conservés sans modification. Les **16 tests C++ M2** :
`event_order`, `rng`, `network`, `network_disorder`, `hash_power`, `local_views`,
`pending`, `duplicates`, `fork_choice`, `reorganization`, `invalid_blocks`,
`reproducibility`, `config_invalid`, `limits`, `metrics`, `demo_fork`.
Deux tests CLI supplémentaires : simulation depuis fichier et fichier absent.
Les tests incluent une branche courte de travail 256 contre une branche de
deux blocs de travail total 32 ; réorganisation de profondeur 3 ; descendants
avant parents ; convergence malgré jitter ; aucune puissance aux observateurs ;
budget événementiel, horizon, capacité de file et budget PoW ; JSON entier identique.

Commandes CLI exécutées avant puis après la dernière compilation, toutes code 0 :

```powershell
./build/ucrt64/blockchain_sim.exe --config configs/baseline.json --output results/raw/m2_baseline
./build/ucrt64/blockchain_sim.exe --config configs/fork_reorg.json --output results/raw/m2_fork
./build/ucrt64/blockchain_sim.exe --config configs/fork_reorg.json --output results/raw/m2_fork_repeat
Get-FileHash results/raw/m2_fork/results.json,results/raw/m2_fork_repeat/results.json
```

| Mesure réellement observée (seed 42) | baseline | fork_reorg |
|---|---:|---:|
| Blocs produits, hors genesis | 20 | 12 |
| Observations locales de fork | 0 | 9 |
| Réorganisations locales | 0 | 4 |
| Messages envoyés / livrés | 80 / 80 | 72 / 72 |
| Doublons reçus | 40 | 48 |
| Échantillons de propagation | 40 | 24 |
| Propagation moyenne (ticks) | 7,3 | 37,5833333333333 |
| Propagation maximale (ticks) | 14 | 58 |
| Parents inconnus à l'arrivée | 0 | 0 |
| Fin | max_blocks_drained | max_blocks_drained |

Le scénario avec forks atteint 12 blocs (Alice 5, Bob 7, observateur 0), puis
converge à hauteur 9 avec 3 blocs stale par vue et aucun parent restant inconnu.
Les 9 observations ne sont pas 9 forks globaux distincts. Les arrivées en désordre
sont réellement provoquées et vérifiées dans `sim_network_disorder`, même si
elles ne se produisent pas dans ces deux exemples.

Trace de réorganisation, profondeur détachée 1 dans chaque cas :

| Tick | Nœud | Hauteur avant → après | Cause |
|---:|---|---|---|
| 61 | bob | 1 → 1 | Départage de travail égal |
| 67 | observer | 1 → 1 | Départage de travail égal |
| 167 | alice | 4 → 5 | Branche de travail supérieur |
| 208 | alice | 6 → 6 | Départage de travail égal |

Les deux fichiers JSON finaux du scénario de fork ont exactement le SHA-256 :

```text
87846FC1590192AFDD3286CF719A057AB3C9EA14AF6EFC4ECC401C3720CBF632
```

Vérifications finales exécutées : `git diff --check` sans erreur ;
`git diff --exit-code -- exo4.cpp` et les six modules M1, leurs tests, le preset
et le script sans différence ; résultats bruts effectivement ignorés par Git.
Empreinte `exo4.cpp` toujours :
`DD167B553F5CFB36D0FA0DCD711E2F6B79B915B4434CBAC3D37EEAA6EC76EAD2`.
Aucun commit, ajout à l'index ni push exécuté.

### Limites et décisions à valider

Réseau fiable à topologie statique, sans bande passante/pertes, transactions
synthétiques sans économie, PoW fixe sans politique contextuelle de difficulté,
pas de synchronisation par requête de parents. Le déterminisme strict est garanti
dans la même construction ; `log` flottant peut différer sur une autre plateforme.
Les exemples n'établissent aucune estimation statistique ni efficacité de défense.

Avant les attaques : valider le départage lexicographique, la résolution en ticks,
la calibration du hash power, la politique de difficulté et les conditions réseau
du protocole scientifique. La portée PoS reste à clarifier contre l'énoncé.
M3, pools, paiements, récompenses, détecteurs, mitigations, analyse Python et web
n'ont pas été implémentés. Prochaine étape : revue/validation de M2, puis définition
du scénario 51 % et de ses règles de paiement/confirmations en M3.

## 2026-11-04 — M3 : quatre attaques, intégration séquentielle

Sources relues : instructions, plan, énoncé officiel, modèle, journal et moteur M2.
Aucune contradiction bloquante : M3 impose quatre comportements distincts,
contrôles honnêtes, assertions d'objectifs et exports bruts. La portée PoS reste
ambiguë dans l'énoncé ; ce travail concerne exclusivement les attaques PoW/P2P.
Le M4 complet (détecteurs, mitigations, évaluation statistique) n'est pas introduit.

### Vérifications réelles, dans l'ordre demandé

Toutes les étapes ont lancé `./scripts/test-ucrt64.ps1`, avec GCC/OpenSSL UCRT64,
CMake/Ninja/CTest existants et `--parallel 2`. Presets et script inchangés.

| Étape | Résultat CTest exécuté | Durée réelle |
|---|---|---:|
| Avant toute modification M3 | 31/31 réussis | 1,34 s |
| Chaîne privée / double dépense | 32/32 réussis | 2,62 s |
| Selfish mining | 33/33 réussis | 2,35 s |
| Pool / block withholding | 34/34 réussis | 2,62 s |
| Sybil | 35/35 réussis | 2,36 s |
| Exports/contrôles/fichiers d'échec | 35/35 réussis | 2,27 s |
| Vérification finale renforcée | 35/35 réussis | 2,63 s |

Aucun échec de compilation/CTest constaté. Deux avertissements à l'étape selfish
(parenthèses et variable inutilisée) ont été corrigés à l'étape suivante ; aucun
avertissement de compilation dans le dernier journal. La démo M1 est restée valide
à chaque étape, avec les mêmes hashes/nonces. Journaux locaux :
`build/m3_selfish.log`, `build/m3_withholding.log`, `build/m3_sybil.log`,
`build/m3_final.log` (ignorés par Git).

Le script exécute exactement les commandes suivantes avec les outils Visual
Studio BuildTools trouvés lors de M1/M2, GCC restant celui de MSYS2 UCRT64 :

```powershell
cmake --preset ucrt64 "-DCMAKE_MAKE_PROGRAM=C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe"
cmake --build --preset ucrt64 --parallel 2
ctest --preset ucrt64 --output-on-failure
./build/ucrt64/blockchain_sim.exe --demo
```

### Fichiers créés et modifiés

Créés : `src/attacks/attack.hpp`, `config.cpp`, `attack_51.cpp`,
`selfish_mining.cpp`, `block_withholding.cpp`, `sybil.cpp`, `results.cpp` ;
`src/sim/mining_pool.hpp` ; `tests/test_attacks.cpp` ; douze configurations :
`attack_51`, `selfish_mining`, `block_withholding`, `sybil`, chacune en `.json`,
`_control.json`, `_failure.json` dans `configs/`.

Modifiés : `CMakeLists.txt` (version 0.3.0, sources/tests), `src/sim/config.hpp`,
`config.cpp` (objet attack optionnel validé), `event.hpp` (shares), `simulator.hpp`,
`simulator.cpp` (points de publication/rétention, pool, réseau effectif),
`src/metrics/json_exporter.cpp` (résultats spécifiques), README et les deux
fichiers de documentation. Pas de modification du noyau, de main.cpp, des tests
M1/M2, des presets ou du script PowerShell. Pas de dépendance supplémentaire.

### Assertions comportementales

Quatre nouveaux cas CTest contiennent chacun plusieurs exécutions et assertions :

- `attack_51` : paiement accepté puis absent de la branche finale, dépense
  concurrente présente, publication et réorganisation ; échec à puissance zéro
  et confirmations impossibles malgré des blocs privés.
- `selfish_mining` : avance privée et publication effectives, part active supérieure
  au poids de hash dans le scénario favorable ; absence de gain/publication à
  puissance zéro ; contrôle honnête sans rétention.
- `block_withholding` : shares plus nombreuses que solutions complètes retenues,
  perte de récompenses canoniques par rapport au contrôle apparié, pas de
  rétention dans ce contrôle ; fichier d'échec à puissance adverse nulle.
- `sybil` : messages filtrés, victime isolée ; échec avec chemin honnête conservé,
  mêmes temps de découverte et même nombre de blocs ; identités de poids positif
  rejetées.

Chaque cas vérifie aussi la reproduction exacte du JSON, le rejet d'un participant
hors limites et son fichier de scénario d'échec. Les 31 tests M2 demeurent inchangés.

### Exécutions CLI observées

Les douze configurations ont été exécutées réellement, code de sortie 0, avec :

```powershell
# Pour chaque NAME parmi attack_51, selfish_mining, block_withholding, sybil,
# puis NAME_control et NAME_failure :
./build/ucrt64/blockchain_sim.exe --config configs/NAME.json --output results/raw/m3/NAME
```

| Scénario seed 42 | Résultat observé |
|---|---|
| 51 % favorable | paiement confirmé renversé ; 3 blocs privés publiés ; success=true |
| 51 % échec | 84 blocs privés, aucune publication faute de confirmations ; success=false |
| Selfish favorable | 52 retenus, 50 publiés ; part active 37/67 = 55,22 % pour 40 % de hash |
| Selfish contrôle | aucune rétention ; part active 48,84 % sur cette seed |
| Selfish échec | puissance adverse nulle ; success=false |
| Withholding favorable | 52 solutions complètes retenues ; récompense pool 48 unités |
| Withholding contrôle | aucune rétention ; récompense pool 99 unités |
| Withholding échec | poids adverse nul ; aucune solution retenue ; pool 100 unités |
| Sybil favorable | 40 messages filtrés ; victime au genesis ; success=true |
| Sybil échec | 40 filtrages mais chemin honnête ; victime reçoit les blocs ; success=false |

Tous les contrôles ont success=false. Les unités de pool valent un bloc actif
unitaire dans la vue cible ; ce ne sont pas des monnaies réelles. Les exemples
ne démontrent pas un gain moyen ou une probabilité d'attaque. Les shares et payouts
sont exportés dans `attack.pool`, la topologie effective dans `attack.effective_links`.
Les traces incluent rétention et publication, en plus des réorganisations M2.

### Limites et décisions scientifiques

Le modèle détaille les choix : jeton unique sans portefeuille/signature/UTXO
générique ; connaissance idéale des confirmations du marchand ; stratégie selfish
simplifiée publiant toute l'avance face à la concurrence (pas Eyal–Sirer complet) ;
shares probabilistes sans calcul de hash bas seuil ; pool sans serveur de jobs,
payout prorata sur le run ; capture des connexions Sybil supposée au démarrage,
victime passive. Une seule attaque par run. Pas d'hypothèse que les défenses
fonctionnent : aucune défense M4 implémentée.

Ces choix sont exécutables et testés mais restent à valider comme protocole
scientifique avant des campagnes : biais du départage lexicographique, résolution
des ticks, topologie, oracle marchand, stratégie selfish et politique de pool.
Les résultats sont censurés par les limites configurées ; les branches privées
restantes ne sont pas publiées artificiellement à la fin.

`exo4.cpp` conserve le SHA-256
`DD167B553F5CFB36D0FA0DCD711E2F6B79B915B4434CBAC3D37EEAA6EC76EAD2`.
Aucun commit, mise en index ou push effectué. Prochaine étape : revue de M3 et des
hypothèses avant M4, sans démarrage de M4 dans cette tâche.

## 2026-11-04 — M4 : détection statistique adaptative et mitigations

Décision du propriétaire appliquée : aucun Machine Learning. Sources relues :
AGENTS, PLAN, projet, modèle, historique, core/sim/attacks/metrics/tests.
Les quatre contre-mesures et les détecteurs fixe puis adaptatif suivent M4.
Dépendances minimales ajoutées : registre public de soumissions par participant,
observations par voisin, groupes administratifs de pairs, reconnexion et rattrapage
local. Aucune connaissance des blocs privés n'est fournie aux détecteurs.
L'état initial contenait les modifications M3 non commitées ; elles sont conservées.

### Étapes et exécutions réelles

Toutes les constructions passent par le script existant `./scripts/test-ucrt64.ps1`,
avec CMake/Ninja, GCC et OpenSSL UCRT64, compilation limitée à **deux tâches**.

| Étape | Résultat réellement exécuté | Durée CTest |
|---|---|---:|
| Avant toute modification M4 | 35/35 réussis | 2,00 s |
| Fenêtres, fixe et confusion | 36/36 réussis | 2,63 s |
| EWMA et seuils adaptatifs | 36/36 réussis | 2,46 s |
| Observations publiques et raccordement | 36/36 réussis | 1,69 s |
| Politique de confirmations, pureté, référence externe | 39/39 réussis | 2,26 s |
| Probation des messages tardifs | 40/40 réussis | 2,12 s |
| Premier audit de pool | 40/41 réussis, security_pool en échec | 2,21 s |
| Audit avec fenêtre suffisante et test explicite du cas manqué | 41/41 réussis | 2,28 s |
| Reconnexion et rattrapage Sybil | 42/42 réussis | 2,29 s |
| Modes de comparaison et paramètres invalides | 43/43 réussis | 2,97 s |
| Ensemble des exemples contrôlés et reproductibilité | 44/44 réussis | 4,90 s |
| Bornes numériques et contrôles finaux | 44/44 réussis | 3,97 s |

L'échec de pool n'a pas été masqué : la fenêtre de 2000 ticks ne produisait pas
assez de shares pour dépasser le seuil 3. Ce cas est désormais un test d'attaque
active **non détectée**. La fenêtre de 10000 ticks, sans abaisser le seuil, permet
l'audit opérationnel et est déclarée dans les exemples pool. Les conditions de
puissance, les limites et la seed restent explicites.
Un avertissement de copie de paire dans la reconnexion a été corrigé ; aucun
avertissement de compilation dans le dernier journal complet.

Commandes réellement lancées par le script, avec les chemins d'outils M1/M2 :

```powershell
cmake --preset ucrt64 "-DCMAKE_MAKE_PROGRAM=C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe"
cmake --build --preset ucrt64 --parallel 2
ctest --preset ucrt64 --output-on-failure
./build/ucrt64/blockchain_sim.exe --demo
```

Les **35 tests M1–M3** demeurent exécutés. Neuf cas CTest nouveaux :
`detection_statistics`, `security_confirmations`, `security_publication`,
`security_pool`, `security_peers`, `security_purity`, `security_calibration`,
`security_configuration`, `security_controlled_examples`. Chaque cas contient
plusieurs assertions et exécutions. Le test d'ensemble génère sa propre référence
dans build/, sans dépendre d'artefacts préexistants dans results/.

Vérifications : fenêtre (t-W,t], Welford, variance/EWMA exactes, seuil calculé
avant mise à jour, plancher de dispersion, calibration absente/insuffisante,
contamination lente, confusion TP/FP/TN/FN/F1, observations identiques avec labels
baseline/attacked/protected donnant les mêmes décisions, scénarios honnêtes
calmes sans alerte pour les deux détecteurs, FP conservés sur les honnêtes bruités,
FN réel avec fenêtre pool courte, effets sur acceptations/messages/crédits/réception,
mode sans défense non opérationnel, paramètres invalides et JSON reproductible.

### Fichiers de cette tâche

Créés (45 fichiers, dont 28 configurations) :

```text
src/detection/detector.hpp / detector.cpp
src/detection/feature_extractor.hpp / feature_extractor.cpp
src/detection/config_io.hpp / config_io.cpp
src/mitigation/policy.hpp / policy.cpp
src/metrics/detection_evaluation.hpp / detection_evaluation.cpp
src/metrics/security_export.hpp / security_export.cpp
src/sim/security.cpp
tests/test_detection.cpp
tests/test_security.cpp
tests/test_security_experiments.cpp
scripts/run-m4-examples.ps1
configs/m4/<famille>_calibration.json                                  (4)
configs/m4/<famille>_<fixed|adaptive>_<baseline|attacked|protected>.json  (24)
```

Familles de noms : attack_51, selfish_mining, block_withholding, sybil.
Modifiés : CMakeLists (version 0.4.0), README, SIMULATION_MODEL et ce journal ;
sim/config.hpp et .cpp, simulator.hpp et .cpp, network.hpp et .cpp, event.hpp,
mining_pool.hpp ; attacks/attack.hpp, attack_51.cpp, results.cpp ;
metrics/json_exporter.cpp. Pas de dépendance supplémentaire.
Core, main.cpp, exo4.cpp, tests existants, presets et script de construction
préservés. Les modifications des sources M3 sont limitées à l'acceptation
marchande et aux registres/exports nécessaires à M4.

### Comparaisons exécutées

`./scripts/run-m4-examples.ps1` a été exécuté réellement. Il lance quatre
calibrations honnêtes seed 7, puis 24 runs évalués seed 42 (4 familles x 2
algorithmes x 3 modes). Tous retournent 0. Les JSON bruts et la synthèse sont
locaux dans `results/raw/m4/`, ignorés par Git. Les baselines évaluées chargent
la référence antérieure : elles ne sont pas artificiellement exemptées d'alertes.

Résultats des exemples attaqués, unité (run, famille principale) :

| Attaque | Fixe : TP/FP/FN, latence | Adaptatif : TP/FP/FN, latence |
|---|---|---|
| 51 % | 1/1/0, 100 ticks | 1/1/0, 100 ticks |
| Selfish | 1/1/0, 100 ticks | 1/1/0, 100 ticks |
| Withholding | 1/1/0, 5200 ticks | 1/0/0, 7900 ticks |
| Sybil | 1/0/0, 100 ticks | 1/0/0, 100 ticks |

Les baselines honnêtes bruitées ont respectivement 2, 2, 2, 0 familles FP en
fixe, et 1, 1, 2, 0 en adaptatif. Les tests de référence calme ont zéro alerte.
Les nombres sont des observations sur ces exemples, pas des estimations de
précision/rappel en population. Une anomalie de publication d'un run 51 % est FP
au sens du classement primaire choisi, sans nier l'anomalie réellement observable.

Effets opérationnels mesurés :

- 51 % : les deux détecteurs alertent après l'acceptation M3 ; la perte reste
  **1 jeton pédagogique**, sans modification du fork choice. Un autre test avec
  forks avant acceptation démontre le report réel du règlement par la politique.
- Selfish : 55,2239 % de blocs actifs sans défense, **44,8276 %** avec probation
  de 100 ticks, pour 40 % de hash power ; 141 reports effectifs de messages.
  L'avantage reste positif et le succès M3 reste true. Le délai initial de 20 ticks
  n'avait pas réduit l'avantage dans cet exemple ; 100 est une configuration
  expérimentale forte, pas un paramètre prétendument optimisé statistiquement.
- Pool : récompense totale **48 unités** dans les deux modes, contre 99 dans la
  baseline appariée. Fixe : 371 shares non créditées, payout adverse de 17,8966
  à **6,9579** unités. Adaptatif : 291 shares non créditées, payout **9,9401**.
  Les solutions retenues ne sont pas récupérées ; succès comportemental M3 true.
- Sybil : deux reconnexions et messages de rattrapage ; la victime reçoit de vrais
  blocs et l'isolement total échoue. Le rattrapage déclenche aussi une famille FP
  de publication, conservée dans le résultat protégé.

Les exemples isolent par commutateurs la contre-mesure étudiée. La politique ne
sélectionne jamais une action à partir du type d'attaque. Une première exécution
avec toutes les contre-mesures actives a montré des interactions (dont une
reconnexion inutile en pool) ; les paramètres finaux isolent leurs effets.

### Hypothèses et limites

Formules, seuils, unité d'évaluation, provenance de calibration, coûts et
contre-mesures sont détaillés dans SIMULATION_MODEL.md. La référence est honnête
mais déclarative ; les fenêtres se chevauchent, leurs échantillons ne sont pas
indépendants. Les seuils par défaut sont des choix d'ingénierie non optimisés.
Les moments sont ancrés pour limiter la contamination ; une dérive normale hors
bande peut provoquer des FP et nécessiter une nouvelle référence honnête.

Les délais annoncés peuvent être falsifiés hors du modèle d'attaques actuel ;
la diversité de groupes ne prouve pas l'honnêteté et dépend d'un annuaire fiable.
Une suspension de crédits peut léser un mineur honnête, une probation un relais
lent. La politique de confirmations ne protège pas une vente déjà acceptée.
Aucun changement du consensus par travail cumulé, aucune défense universelle,
aucun détecteur ML, aucune campagne M5, analyse Python, backend ou frontend.
CPU réel/RSS non mesurés : les coûts sont des événements/messages/échantillons
logiques et des délais simulés, distincts des temps réels de CTest.

Le protocole avant généralisation devra valider les seuils, groupes de confiance,
coûts de probation/suspension et objectifs marchands ; les résultats multi-seeds
restent à M5. Le périmètre PoS de l'énoncé reste non résolu.
Aucun commit, mise en index ou push effectué.

Vérification finale après renforcement du test de séparation des labels :
**44/44 tests réussis, 0 échec, 3,70 s**, script de construction code 0,
sans avertissement de compilation. `git diff --check` sans erreur de contenu
(avertissements Git LF/CRLF seulement). Le noyau, main.cpp, exo4.cpp, presets et
script de test n'ont aucune différence ; SHA-256 d'exo4.cpp toujours
`DD167B553F5CFB36D0FA0DCD711E2F6B79B915B4434CBAC3D37EEAA6EC76EAD2`.
Les comparaisons CLI ont été relancées sur la version finale fonctionnelle ;
le dernier renforcement ne modifie que les assertions de tests.

## 2026-11-04 — M5 : benchmark sur 30 seeds, M4 strictement gelé

Demande du propriétaire appliquée : évaluation uniquement. Sources relues :
AGENTS, PLAN, projet, SIMULATION_MODEL, journal, README, configurations M4,
orchestrateur M4, métriques, détecteurs et politiques. Les scripts Python et les
figures sont autorisés explicitement par cette demande M5. Aucune source C++,
configuration M4, stratégie, seuil ou mitigation n'a été modifié. Aucun build
ni CTest relancé : les 44 tests M1–M4 étaient déjà validés.

### Protocole et gel de référence

- Commit : `9cd1db52813cdb3fc03ac21fc2a3aab6256ce1d3`.
- `analysis/m5_reference.json` enregistre les SHA-256 de 106 fichiers d'entrée
  et cinq binaires/DLL, l'état Git, l'environnement et le préfixe historique
  intégral de ce journal. Le gel couvre aussi les fichiers hors suivi Git.
- Binaire exécuté : SHA-256
  `933ed797816cd66d4ec162536f29d6ebf9f1f7221f685da47f3a782f7d092179`.
- Seeds finales préenregistrées dans `analysis/m5_protocol.json` : **1001 à 1030**
  incluses, 30 distinctes, sans 7 ni 42. Quatre profils M4, deux détecteurs,
  trois modes : **720 évaluations**.
- Quatre calibrations honnêtes seed 7, configurations M4 de calibration conservées.
  Une référence gelée par profil, partagée entre les 90 évaluations adaptatives
  correspondantes. Aucun apprentissage sur les données attaquées/protégées.
- Les configurations temporaires changent uniquement seed et chemin de référence.
  Hash power, topologie, difficultés, fenêtres, seuils et budgets sont inchangés.
- Une seule simulation C++ à la fois, jamais de multiprocessing ; aucun graphique
  pendant les simulations. Traitement d'un JSON détaillé à la fois, résumés légers
  seulement en mémoire pour l'analyse.

### Automatisation et vérifications réalisées

Nouveaux scripts sous analysis/ : benchmark.py, aggregate.py,
audit_statistics.py, test_benchmark.py, test_statistics.py ; protocole, preuves,
paramètres archivés et README_M5.md. Aucun fichier M4 remplacé.
L'orchestrateur utilise un verrou Windows, un manifeste JSONL avec fsync,
des empreintes par résultat/configuration/résumé et des tentatives distinctes.
Ctrl+C termine le processus actif ; une reprise vérifie les artefacts avant de
les sauter. Les erreurs restent enregistrées et ne deviennent pas des résultats
fictifs. Le budget du moteur n'est jamais augmenté.

Commandes réellement exécutées avec Python 3.11.9 installé :

```powershell
$m5Python = 'C:\Users\LENOVO\AppData\Local\Programs\Python\Python311\python.exe'
& $m5Python -B -m unittest discover -s analysis -p 'test_*.py' -v
& $m5Python -B analysis/benchmark.py freeze
& $m5Python -B analysis/benchmark.py pilot
& $m5Python -B analysis/benchmark.py run
& $m5Python -B analysis/benchmark.py repair
& $m5Python -B analysis/aggregate.py
& $m5Python -B analysis/audit_statistics.py
& $m5Python -B analysis/benchmark.py verify
```

Le pilote a exécuté quatre calibrations, deux évaluations distinctes (seeds
9001/9002) et une répétition 9001 : sept processus réussis, exclus des 720.
La répétition donne exactement le même SHA-256. La reprise du pilote a lancé
zéro simulation et reconnu les sept artefacts valides.

La campagne principale a lancé **720 processus C++, tous code 0**. Une erreur de
l'extracteur Python (`hash` au lieu de `block` pour la trace de probation) a
empêché les résumés de 60 runs selfish protégés. Les données brutes étaient
présentes et valides. Après correction de l'extracteur et ajout d'un test,
`repair` a reconstruit ces 60 résumés **sans relancer le C++** ; les événements
d'erreur initiaux restent dans le manifeste. Aucun résultat ni seed supprimé.
La reprise finale a lancé zéro C++ et validé 724 artefacts (720 + 4 calibrations).

Bilan exact : **720/720 évaluations valides**, 30 par chacune des 24 combinaisons ;
4 calibrations + 3 pilotes = **727 lancements C++ au total**, zéro échec de processus.
**Dix tests Python réussis** (orchestration, reprise/journal tronqué, validation,
bootstrap, valeurs connues, manque de données, appariement, micro/macro et Wilson).
L'audit indépendant stdlib vérifie 1272 lignes statistiques, 1484 comparaisons
appariées et les paramètres/empreintes des **727 entrées réellement exécutées**.
Les processus fixes/adaptatifs sans mitigation ont aussi des signatures de
production de blocs identiques par seed.

### Restitution et résultats principaux

Rapport : `analysis/M5_BENCHMARK_REPORT.md`. Quatre tables CSV sous
analysis/tables/ : 720 résumés, 24 campagnes, statistiques complètes et différences
appariées. Six figures PNG et SVG sous analysis/figures/. Données détaillées et
manifeste conservés dans `results/raw/m5_30seeds/`, ignorés par Git.
Les statistiques utilisent 5000 bootstrap par seed (seed de bootstrap 20261104),
IC 95 %, moyennes/médianes/écarts-types/min/max et effectifs disponibles.
Les différences sont formées avant bootstrap. Les 720 runs ne sont pas traités
comme indépendants d'un même effet ; les familles ne sont pas des attaques
indépendantes. Les latences non détectées restent censurées et non imputées.
Les proportions 0/30 ou 30/30 ont aussi un IC Wilson, car le bootstrap peut être
dégénéré. Les IC restent conditionnels à la calibration M4 unique par profil.

- **51 %** : 30/30 doubles dépenses réussies, avec et sans défense, pour les deux
  détecteurs ; perte moyenne 1 jeton. Le fixe détecte 30/30, l'adaptatif 28/30
  (deux FN). Latences conditionnelles moyennes 273,3 et 321,4 ticks. La défense
  marchande arrive après l'acceptation ; le modèle n'a pas été retouché.
- **Selfish** : succès au sens M3 10/30 attaqués, 4/30 protégés. Part active moyenne
  **38,97 % → 26,39 %**, contre 40 % de hash power. Différence moyenne d'environ
  **−12,59 points**, IC bootstrap 95 % environ [−15,9 ; −9,1]. Le modèle selfish
  n'est pas rentable en moyenne au sens de ce proxy sur ces seeds. Sur **3/30**
  paires, la défense augmente pourtant la part adverse. La seed 42 n'était donc
  pas représentative d'un gain moyen garanti. Hausse des stale et reports de
  contenus honnêtes conservés parmi les coûts.
- **Withholding** : 39,57 solutions complètes retenues en moyenne. Récompense pool
  98,63 en baseline, **60,43** attaquée et protégée : les blocs ne sont pas récupérés.
  Payout adverse 24,31 sans défense, **8,18** en fixe et **11,92** en adaptatif.
  Détection moyenne 4660 contre 7300 ticks ; précision micro 0,50 contre 0,75
  sur les runs attaqués, rappel 1. Aucune share honnête non créditée sur ces
  30 runs par détecteur, sans prétendre à un risque nul.
- **Sybil** : isolement 30/30 sans défense, **0/30** protégé. Adoption valide après
  reconnexion : moyenne 39,13 ticks, médiane 4,5, plage 1–176 (n=30). Coût réseau
  moyen supplémentaire ~109,17 messages. Les groupes supposés fiables et le
  bootstrap restent des hypothèses du modèle, pas une garantie sur un vrai P2P.

Les baselines bruyantes comportent des FP : totaux de familles FP fixe/adaptatif
48/12 (51 %), 57/20 (selfish), 53/45 (pool), 17/17 (Sybil), sur 120 unités de
classement mais 30 clusters indépendants par cas. Ces erreurs sont conservées.
Le rapport distingue les FP de classement primaire des anomalies réellement
observables d'une autre famille.

### Ressources et limites documentées

Mesures réellement prises : **84,500 s de CPU C++ cumulé** (727 processus,
GetProcessTimes), maximum RSS C++ échantillonné à ~10 ms **13,44 MiB**,
RSS observée entre runs de l'orchestrateur **29,46 MiB**, données brutes de
campagne **127,13 MiB** au moment de l'analyse. Ces RSS ne sont pas des garanties
de pic exact. Les durées réelles dépendent aussi de la machine ; les mesures de
coût CPU/wall sont exploratoires, distinctes des ticks simulés.

NumPy/pandas/psutil étaient déjà présents. Pour tracer les figures, Matplotlib
et ses petites dépendances ont été téléchargés après autorisation et installés
uniquement dans `build/m5-python`, sans installation globale ni dépendance du
moteur. Versions enregistrées dans analysis/requirements-m5.txt et m5_integrity.json.

Non disponibles exactement dans les exports : confirmations au moment de
l'acceptation, effets spécifiques sur chaque relais émetteur honnête, blocs
uniques attribuables exclusivement au rattrapage. Les métriques/proxies disponibles
et les raisons sont explicités. Le coût d'une fausse sanction sur un scénario
entièrement honnête protégé n'est pas isolé par les trois modes M4 de référence.
Aucun budget augmenté ni scénario supplémentaire inventé pour combler ces limites.

Les empreintes M4 et le préfixe historique de ce journal sont inchangés.
Aucun commit, push, changement de M4, FastAPI ou frontend. La tâche s'arrête
à la livraison des benchmarks pour examen personnel du propriétaire.


## 2026-11-06 — Reprise PoW, terminal et politique marchande 0.5.0

Décision explicite du propriétaire : laboratoire des quatre attaques PoW uniquement, terminal simple/personnalisé, aucun PoS distribué/web/ML. L’énoncé mentionne PoW/PoS ; cette limitation est déclarée dans PLAN/AGENTS/README/modèle, sans extrapolation. Aucun commit ni push.

Avant modification : Git propre au commit 5376e2c, objet Git de référence M4 9cd1db52813cdb3fc03ac21fc2a3aab6256ce1d3 disponible. `./scripts/preserve-m5.ps1 -Create` a vérifié et archivé 142 fichiers (sources/docs/configs/analyse, exécutable et DLL), et enregistré/vérifié les empreintes de 2914 fichiers M5 historiques. Archive locale ignorée `build/m5-reference`, à conserver : pas une sauvegarde distante. Le script refuse toute divergence ; le moteur refuse les sorties vers m5_30seeds/m5-reference. Le vérificateur M5 ancien s’applique désormais à son arbre archivé, pas aux sources 0.5 modifiées.

Non-régression avant code : `./scripts/test-ucrt64.ps1` a réellement compilé/vérifié GCC UCRT64 et exécuté **44/44 tests réussis, 4,29 s**, démo M1 valide. CMake/CTest 3.31.6-msvc6 et Ninja 1.12.1 de Visual Studio pilotent GCC/OpenSSL UCRT64 comme auparavant ; deux tâches de compilation maximum.

Implémentation : `--interactive`, wizard à flux testable, retours/annulation/validation, génération uniforme de poids 10..100 avec RNG séparé, groupes adverses à ressources non dupliquées, liens manuels ou topologies usuelles, paramètres contextuels, calibration honnête adaptative préalable, exports dans un nouveau run, rejeu/comparaison. Réutilisation du moteur, M1/exo4 inchangés. La hauteur cible concerne un observateur local ; elle ferme la production puis draine la file sous limites, sans publication privée forcée. Les traces pédagogiques affichent parents/blocs privés, réceptions, forks/réorganisations, alertes/actions ; elles ne sont pas des entrées de détection.

Politique : hold_ticks et confirmation_ticks indépendants, seuil encore applicable après suspension, remise à zéro après expiration et renouvellement sur alerte. Audits locaux événementiels optionnels ; signaux précoces facultatifs fork_rate/silence, sans information privée ni future. Acceptations exportées avec confirmations observées/requises et séquence. Les nouveaux scénarios utilisent une annonce d’acceptation P2P, puis la vue publique locale adverse ; les anciens JSON conservent explicitement l’oracle historique public_notice=false. La stratégie générale de rétention/publication n’est pas réoptimisée pour favoriser la défense. Coalition : partage privé instantané supposé ; pas de coût réseau interne. Ce changement d’observation/version interdit de présenter les nouveaux runs comme une réexécution identique de M5.

Validation intermédiaire : une erreur de portée introduite dans un test a été corrigée ; l’assertion de schéma a été passée de 1 à 2 ; une référence temporaire dans minmax a été corrigée suite au warning GCC. Le test historique d’expiration est adapté à la persistance désormais voulue. Suite ensuite à 53/53 (5,71 s), puis 54/54 (7,24 s). Dernière exécution du script après ajout du test d’acceptation effective sous seuil renforcé : **54/54, zéro échec, 9,57 s**, aucune alerte de compilation relevée dans build/final-tests.log. Les dix cas supplémentaires couvrent les deux modes, retour/validation, quatre attaques, coalition/pool, replay/seeds, calibration/insuffisance, arrêt par hauteur, politique, intervention précoce/tardive, faux holds honnêtes, non-acceptation et notification publique/isolement. Les trois autres mitigations passent les contrôles existants. Le terminal a aussi été réellement exercé via stdin : seed42, 4 nœuds/2 mineurs, hauteur8, attaque51 protégée, 13 découvertes, résultat non concluant (branche privée restante), paiement accepté à 2/2 confirmations ; aucun succès artificiel annoncé.

Évaluation séparée : `./scripts/run-validation.ps1` a préparé **1687 jobs sans exécution** sous results/raw/pow_validation_v2/run_2 (1680 évaluations, 7 calibrations). Puissances 0/10/30/49/51/65/85 %, seeds2001..2030, calibration3001, fixe/adaptatif, quatre modes dont honnête protégé. Le protocole prospectif définit appariement, censures, faux positifs et limites ; aucune estimation complète n’est calculée ici.

`./scripts/run-validation.ps1 -Smoke -Run` : **8 processus terminés code0**, distincts de M5, sous results/raw/pow_validation_v2/run_1. Une seule seed2001, détecteur fixe : q30 attaqué/protégé restent non concluants avec branche privée non publiée ; q85 réussit dans les deux modes (paiement accepté tick1698, seuil6), malgré les actions tardives. Les contrôles honnêtes acceptent. Ces observations sont des sanity checks, pas une estimation statistique ni une garantie de défense. Aucune des 720 simulations historiques relancée.

Limites : stratégies M3 simplifiées et une seule stratégie coordonnée par run, modèle de paiement à jeton unique sans signatures, hypothèses Sybil de capture/bootstrap/groupes fiables, PoW témoin borné pouvant épuiser son budget, une référence honnête par profil et métriques partiellement absentes, fenêtre/ticks arrondis, protection sans garantie majoritaire et coût possible d’indisponibilité. L’archive locale doit être sauvegardée hors du PC par le propriétaire s’il le souhaite. Prochaine étape : examiner le protocole puis exécuter séparément la campagne prospective ; aucun résultat M5 n’est remplacé.

Fichiers de cette reprise (état avant cette entrée) :

```text
 M AGENTS.md
 M CMakeLists.txt
 M PLAN.md
 M README.md
 M docs/SIMULATION_MODEL.md
 M src/attacks/attack.hpp
 M src/attacks/attack_51.cpp
 M src/attacks/config.cpp
 M src/attacks/results.cpp
 M src/attacks/selfish_mining.cpp
 M src/cli.cpp
 M src/detection/config_io.cpp
 M src/detection/detector.cpp
 M src/detection/detector.hpp
 M src/metrics/json_exporter.cpp
 M src/metrics/json_exporter.hpp
 M src/mitigation/policy.cpp
 M src/mitigation/policy.hpp
 M src/sim/config.cpp
 M src/sim/config.hpp
 M src/sim/event.hpp
 M src/sim/security.cpp
 M src/sim/simulator.cpp
 M src/sim/simulator.hpp
 M tests/test_security.cpp
 M tests/test_simulation.cpp
?? analysis/POW_VALIDATION_V2_PROTOCOL.md
?? configs/interactive/attack_51.json
?? configs/interactive/validation_protocol.json
?? scripts/preserve-m5.ps1
?? scripts/run-validation.ps1
?? src/terminal/interactive.cpp
?? src/terminal/interactive.hpp
?? tests/test_terminal.cpp
```

Verification finale : scripts/preserve-m5.ps1 confirme 142 fichiers archives et 2914 fichiers historiques identiques, y compris rapports/configurations M5 et exo4.cpp. Le verificateur original execute depuis build/m5-reference/analysis/benchmark.py verify retourne Frozen M4 unchanged ; la demo du binaire archive retourne chain_valid=true. Aucun commit/push.


## 2026-11-06 — Terminal 0.5.1 : lisibilité, ressources et quatre attaques

Travail conservé depuis la reprise 0.5.0, sans redémarrage de l’implémentation ni commit/push. Avant modification : 54/54 tests réellement réussis (7,75 s) et référence M5 vérifiée (142 fichiers archivés, 2914 fichiers historiques). Aucun fichier M5 ou exo4.cpp modifié.

Menus sectionnés, aide ?/aide (défaut et bornes), retour/annulation conservés. Puissance adverse simple explicite 0..100 pour 51 %/selfish/withholding, complément honnête normalisé et aucune double comptabilisation ; paramètres de pool/identités Sybil contextuels. Ressources personnalisées conservées. Récapitulatif avec modification d’une question choisie, réseau/latences, détection, seed et limites. Le choix adaptatif reste la valeur proposée des paramètres avancés. Un réseau manuel partitionné n’est pas présenté comme un anneau connecté.

Affichage synthétique par défaut, détaillé optionnel. Nouveau renderer en lecture seule, aucune mutation du moteur/RNG, égalité JSON testée entre affichages. Les réceptions/envois/retransmissions sont conservés dans l’export ; les alertes/actions répétées sont regroupées seulement à l’écran. Traces publiques du paiement accepté/renversé/restauré, sans changement de stratégie.

Diagnostic exact de l’ancien run utilisateur results/raw/interactive/run_2 : puissance 34/498=6,827309 %, 202 alertes (141 late_p95, 31 reorg_depth, 30 silence_ticks), 31 actions. Détecteur fixe sans calibration. Délais moyens46,1724/max91 > seuil40 via l’anneau honnête, adversaire jamais publié. Les silences finaux viennent des timers inactifs après arrêt (production2349, livraison2407, ancien dernier tick5312). Annulation désormais sans avance d’horloge ; phases production/drain/final_observation exportées, observations finales sans action. Aucun seuil abaissé ni aucune mitigation M4 réoptimisée.

Rejeu du JSON exact : mêmes blocs et délais, acceptation334 conservée, fin2407, 178 alertes dont7 finales, 30 actions. L’audit périodique déjà planifié après fermeture n’est plus exécuté. Avec post_target_ticks=2000 : production normale supplémentaire, 36 découvertes/hauteur31, deux blocs privés non publiés et résultat toujours non concluant. La fenêtre post-cible est prédéfinie et bornée ; pas de prolongation jusqu’au succès, publication forcée ou succès défensif déduit d’une non-acceptation. Les motifs d’arrêt et limites sont distincts de l’issue ; une réussite déjà observée n’est pas effacée par un plafond atteint.

Tests par lots : menu54/54 (6,01 s), arrêt54/54 (6,65 s). Le premier test de censure exigeait à tort une issue non concluante alors que l’objectif pouvait déjà être observé ; correction du cas pour tester une tentative non résolue (confirmations10000), sans changer les stratégies. Dernière exécution réelle `./scripts/test-ucrt64.ps1` : **60/60 tests réussis, zéro échec, 16,34 s**. Deux tâches de compilation maximum, GCC/OpenSSL UCRT64, CMake/Ninja/CTest existants. Les six nouveaux cas couvrent pourcentages, aide/récapitulatif/correction, détection choisie, invariance d’affichage, fenêtre/censure/annulation et les quatre profils simples/personnalisés, avec entrées invalides par attaque. Les autres mitigations passent la non-régression.

`./scripts/check-terminal.ps1` a réellement exécuté 14 sessions CLI code0 (8 simples sans/avec défense, 4 personnalisées, 2 contrôles majoritaires), seed42, entrées/stdout/manifeste SHA-256 sous results/raw/terminal_review/checks_1. Résultats simples : 51 % à40 réussit dans les deux modes (acceptation419, publication425, renversement/alerte431) ; selfish part60,42→55,32 %, toujours au-dessus de40, avec247 reports dont118 contenus honnêtes ; pool47 solutions retenues et récompense44 inchangées, payout adverse16,3729→6,7622,335 shares adverses non créditées ; Sybil isolement réussi sans défense/plafond300 atteint, échoué avec défense après2 reconnexions (510) puis adoption511. Le cas personnalisé Sybil capture=false échoue sans aucune action, car une liaison honnête demeure : ne pas attribuer cet échec à la défense.

Deux autres sessions CLI : 30 nœuds/10 mineurs/15/40 %/+2000 restent non concluants (44 découvertes,16 blocs privés non publiés,399 alertes/84 actions,144 lignes terminal) ; une session adaptative simple5/2/8/40 % exécute la calibration honnête préalable puis l’attaque (23 alertes/10 actions, double dépense réussie). Aucune promesse d’efficacité ni estimation statistique. Les conditions d’arrêt dépendent de la trajectoire ; Sybil n’a pas le même horizon réalisé dans ses deux modes. Une campagne à budget commun reste nécessaire pour des conclusions statistiques.

Rapport complet : analysis/TERMINAL_REVIEW_051.md. Documentation README, modèle et plan mis à jour. Créés dans ce lot : src/terminal/display.hpp/.cpp, scripts/check-terminal.ps1, analysis/TERMINAL_REVIEW_051.md. Modifiés dans ce lot : CMakeLists.txt ; src/terminal/interactive.hpp/.cpp ; src/sim/config.hpp/.cpp, event.hpp, event_queue.hpp, simulator.hpp/.cpp, security.cpp ; src/metrics/json_exporter.cpp, security_export.cpp ; tests/test_terminal.cpp ; PLAN.md, README.md, docs/SIMULATION_MODEL.md et ce journal. Les autres modifications déjà présentes du lot précédent sont conservées.

Limites : seuils fixes peu adaptés à certains diamètres/cadences, alertes locales répétées et coût honnête des sanctions, stratégies M3 simplifiées, bootstrap/groupes Sybil supposés fiables, une seed par essai. Aucun test isolé ne démontre une efficacité générale, et aucune garantie contre une majorité de hash power n’est donnée. Aucun M5 relancé. L’empreinte du run utilisateur original reste inchangée.


## 2026-11-06 — M4 0.6.0 : raffinement scientifique par lots

Hypothèses/critères/coûts/tests consignés avant implémentation dans docs/SECURITY_REFINEMENT_PLAN.md. Périmètre : M4 seulement, instrumentation publique/intégration événementielle nécessaire, aucune modification des stratégies M3. L’empreinte initiale de tous les fichiers M1/M3, exo4.cpp et configurations préexistantes est conservée dans build/security_before_hashes.json ; vérification finale : zéro divergence. Les modifications déjà non commitées des travaux antérieurs sont conservées. Aucun commit/push.

Avant : scripts/preserve-m5.ps1 vérifié et 60/60 CTest réellement réussis (21,92 s). Lot A : 61/61 (23,26 s). Lot B : 62/62 (23,38 s). Lot C : 65/65 (37,07 s), puis dernière suite après précision du compteur causal : **65/65, zéro échec, 27,45 s**, aucun warning/erreur de compilation relevé. Commande : ./scripts/test-ucrt64.ps1, GCC/OpenSSL UCRT64, CMake/Ninja/CTest existants, deux tâches maximum.

A : séparation suspension/confirmations déjà présente, conservée et testée ; max_payment_hold_ticks optionnel (0=historique), confirmations seules possibles avec hold0. Journal marchand public avant évaluation M3, avec tête/présence/confirmations/suspension/éligibilité, séquence et ordre de trace. Sans acceptation, les compteurs avant acceptation sont null et les actions pendant la non-acceptation restent comptées. Base-ready sur trajectoire réalisée n’est pas un contre-factuel. Signal précoce190, occasion206, acceptation377 sous seuil renforcé. Cas tardif85% : acceptation74, action85, double dépense réussie. Stress honnête (silence1/cooldown1 volontairement artificiels) : refus sans plafond, acceptation512 avec plafond50, contrôle206. Ce gain de disponibilité coûte306 ticks et ne prouve pas une amélioration de sûreté. Pas de modification de l’attaque ni de seuils de benchmark pour fabriquer un succès.

B : références honnêtes liées au profil réseau initial, strict_reference et stratify_reference optionnels, anciennes références non liées compatibles sauf demande stricte. Nouveaux fichiers de référence schéma2 et contrôle de source honnête. Profil sans seed/activation défensive, avec ressources nominales, topologie/délais/cadence/fenêtres et mécanismes publics ; ce contrôle est hors ligne, pas une estimation de puissance adverse cachée. Strates publiques nœud/composant/métrique ; EWMA/seuil fixe/clipping inchangés. Train41001 et validation42001 indépendants sur30 nœuds : alertes826→265 (retards544→14), mais227 réorganisations restent alertées et336 évaluations sont ignorées faute de strate ;227 cellules de référence. À couverture commune,822 alertes fixes contre265 stratifiées ;4 alertes fixes étaient hors couverture. Les deux modes ont les mêmes blocs et les mêmes trois familles de faux positifs sur ce run honnête. Interventions honnêtes258→227, paiement119 dans les deux modes : bruit réduit, aucune amélioration de disponibilité démontrée ici ni taux de faux positifs général.

C : coûts de probation par vrai émetteur/producteur, étiquettes appliquées uniquement après simulation ; crédits/payouts de pool par rôle, perte par contrôle apparié ; accès bootstrap public restreint/retardé en option, sans sélection par label secret. Profils M4 existants seed42 : selfish55,22→44,83% avec141 reports,94 émetteurs honnêtes et46 contenus honnêtes (catégories recouvrantes),4600 ticks ajoutés à ces contenus. Pool99 unités honnêtes vs48 attaquées/protégées, donc51 perdues, distinctes des52 solutions retenues ; payout adverse17,8966→6,9579,371 shares adverses non créditées et0 honnête dans ce cas. Le test sensible honnête (.01/min_shares1, pas une recommandation) observe767 shares honnêtes sanctionnées. Sybil : demande100/adoption101 si candidat accessible, adoption351 si connexion retardée250 ; cinq demandes et aucune récupération si seuls relais contrôlés accessibles. Échec de mitigation conservé, pas réparation omnisciente ni délai nul imputé.

Cinq exemples JSON neufs sous configs/security_refinement/ réellement exécutés, tous code0, sorties results/raw/security_refinement/ : marchand plafonné, calibration/validation réseau, bootstrap restreint et retardé. Les résultats des cas de développement sont sous build/ucrt64/security-refinement/. Les coûts CPU/RSS propres au collecteur ne sont pas profilés ; seuls les records logiques/messages/ticks sont mesurés. Une référence de profil ne résout pas les changements de conditions après calibration ; abstentions et strates insuffisantes doivent rester visibles.

Rapport : analysis/M4_REFINEMENT_REPORT.md. Proposition indépendante non exécutée : analysis/M4_INDEPENDENT_BENCHMARK_PROPOSAL.md,16 profils, puissances/topologies/accès distincts, ensembles disjoints et quatre/six conditions,4680 runs finaux nominaux et6432 avec entraînement/validation/pilotes. Validation du propriétaire et estimation disque/temps obligatoires avant la grande campagne. Le merge multi-seeds des moments nécessite encore une implémentation/test dédiée avant de prétendre à une calibration multi-seeds. Aucune des720 simulations M5 relancée.

Créés : src/metrics/security_study.hpp/.cpp ; src/detection/profile.hpp/.cpp ; tests/test_security_refinement.cpp, test_network_reference.cpp, test_defense_costs.cpp ; cinq configs/security_refinement/*.json ; docs/SECURITY_REFINEMENT_PLAN.md ; analysis/M4_REFINEMENT_REPORT.md et M4_INDEPENDENT_BENCHMARK_PROPOSAL.md. Modifiés dans ce lot : CMakeLists.txt ; src/detection/detector.hpp/.cpp et config_io.cpp ; src/mitigation/policy.hpp/.cpp ; src/sim/simulator.hpp/.cpp, security.cpp, config.cpp, event.hpp ; src/metrics/json_exporter.cpp, security_export.cpp ; README.md, PLAN.md, docs/SIMULATION_MODEL.md et ce journal. Les autres fichiers déjà modifiés des lots précédents n’ont pas été réécrits.


## 2026-11-06 — Simplification finale du terminal uniquement

Modifications limitées à la couche terminal, ses tests/saisies de régression, CMake et documentation. Empreintes avant/après : src/core, sim, attacks, detection, mitigation, metrics, toutes configurations et tous fichiers analysis existants, exo4.cpp : aucune divergence (build/simple_ui_preservation.json). Aucune nouvelle fonctionnalité scientifique, aucun seuil ou benchmark modifié, aucune campagne lancée, aucun commit/push.

Mode simple réduit aux six paramètres demandés (sans puissance pour Sybil), récapitulatif court et confirmation/correction. Plus de réglages de pool/identités/réseau/fenêtre/détails en simple ; réglages personnalisés conservés. Les valeurs automatiques utilisent le générateur et les paramètres déjà existants. L’exécution simple, calibration incluse, n’affiche que Simulation en cours... ; exports JSON complets préservés.

Résultats : issue du moteur inchangée, explication courte, défense NON EVALUABLE avant comparaison, trois indicateurs au maximum par attaque. Critères de défense explicités dans docs/TERMINAL_SIMPLE.md et l’écran comparatif. Atténuation économique/selfish n’est jamais présentée comme échec de l’attaque ; non-acceptation et tentative non concluante n’impliquent pas protection. La perte de pool exige une référence honnête réellement exécutée.

Menu persistant : Comparer exécute deux nouveaux runs appariés (même seed/configuration, mode de défense seul changé), partage la référence adaptative exacte et retourne au menu applicatif sans retour à PowerShell. Les tests exécutent ce parcours pour les quatre attaques et pour l’adaptatif ; vérifient les configurations exportées et l’absence de ticks/séquences/nœuds/alertes internes. Test réel de l’exécutable via stdin également réussi (build/simple-app-demo.log) : attaque51 à40 % réussie dans les deux variantes, défense NON, aucune victoire inventée.

Vérifications : suite intermédiaire65/65 réussie (22,77 s), finale ./scripts/test-ucrt64.ps1 : 67/67 réussis, zéro échec,30,83 s, CMake/Ninja/GCC UCRT64, deux tâches maximum. Pas de warning de compilation relevé. Nouveaux tests terminal_simple_app et terminal_simple_verdict. Les saisies du script check-terminal.ps1 sont adaptées, sans changer ses paramètres scientifiques ni relancer ses scénarios.

Créés : src/terminal/simple.hpp/.cpp et docs/TERMINAL_SIMPLE.md. Modifiés dans ce lot : src/terminal/interactive.hpp/.cpp, tests/test_terminal.cpp, CMakeLists.txt, scripts/check-terminal.ps1, README.md et ce journal. Le mode personnalisé et la CLI historique restent disponibles.


## 2026-11-06 — Renforcement final M4, politique optionnelle hardened_m4_v1

Protocole prospectif : docs/FINAL_DEFENSE_PROTOCOL.md ; résultats détaillés et inventaire : docs/FINAL_DEFENSE_RESULTS.md. Ajout C++ effectif de confirmations préventives, épisodes de hold non renouvelés inutilement, niveaux information/persistance/criticité, probation par producteur public avec nouveauté requise, séquestre temporaire de shares et restitution, diversité préventive du client public désigné avec redondance et décompte de l'éclipse initiale. Ne pas reconfigurer les voisins adverses ; ne pas réagir sur les confirmations après l'acceptation publique de l'unique paiement du modèle. Paramètres nouveaux validés et sérialisés ; anciens JSON et terminal inchangés, activation explicite hardened_security=true.

Avant : 67/67 CTest,30,92 s via scripts/test-ucrt64.ps1. Deux rebuilds bloqués à l'édition de liens de l'ancien blockchain_sim.exe (Permission denied) ; contournement local par CMAKE_RUNTIME_OUTPUT_DIRECTORY=build/ucrt64/final-bin, mêmes preset/outils/périmètre. Dernier build réel avec deux jobs et CTest : 72/72,37,43 s, aucun échec, pas de warning/erreur dans le dernier build. Nouveau binaire --demo : chain_valid=true. Tests ciblés initiaux5/5,8,72 s puis non-régression après corrections de comptage des connexions entrantes, des actions post-acceptation et du coût de diversification. Commandes exactes et logs dans le rapport.

Seeds42/43, huit paires attaquées et huit paires honnêtes ; comparaison JSON ne diffère que par security.mode dans chaque paire attaquée, mêmes découvertes, observation fixe5000/40000/3000ticks selon profil. Aucune modification M3 ni résultat sélectionné pour embellir les chiffres. 51%85% : deux doubles dépenses sur deux encore observées ; coût honnête127/102ticks. Selfish40% : part47,16/32,93 ->28,29/10,32%, aucun contenu honnête directement retardé mais hauteur totale réduite24/12blocs, issues protégées inconclusives à horizon fini. Pool : payout adverse immédiatement distribuable42,6241/45,5223 ->8,2968/9,1136 ;1047/1131crédits restituables en séquestre, aucune sanction honnête sur les références ; pertes de pool93/71inchangées. Sybil : isolement initial censuré3000 ->lien indépendantt0, premierbloc48/304 ;2liensajoutés et96/100messages honnêtes supplémentaires (+50%). Annuaire uniquement contrôlé : récupération impossible conservée. Pas de promesse d'efficacité générale.

SHA-256 des108fichiers préexistants protégés : zéro divergence ; archive M5 :142fichiers et2914sorties historiques vérifiés. Exo4, core, stratégies d'attaque, hasard, fork choice, terminal et anciens configs/analysis préservés. Aucun commit/push et aucune campagne M5 relancée. Les modifications des tâches antérieures restent non commitées.


## 2026-11-06 — Activation finale dans le terminal simple

Périmètre : src/terminal/interactive.cpp, simple.hpp/.cpp et tests/test_terminal.cpp uniquement pour le comportement ; mode personnalisé et générateur commun inchangés. Le formulaire simple active hardened_security pour fixe/adaptatif et le désactive sans défense. Comparer en simple force false/true sur les deux variantes, même après une sélection initiale sans défense. Les références honnêtes adaptatives sont calculées séparément car les extracteurs v2/v3 ont des profils incompatibles ; aucune référence historique n'est réétiquetée. Le chemin personnalisé conserve ses règles antérieures.

Build réel : cmake --build --preset ucrt64 --parallel 2 (sortie locale final-bin déjà configurée). CTest ciblé : ctest --preset ucrt64 -R '^terminal_(simple|custom|simple_app|adaptive)$' --output-on-failure : 4/4 réussis, 10,42 s. Tests couvrant les quatre attaques depuis Comparer et l'adaptatif, activation fixe/adaptative, désactivation sans défense et conservation du personnalisé. Exécution supplémentaire du vrai build/ucrt64/final-bin/blockchain_sim.exe --interactive par stdin, initialement sans défense puis Comparer dans l'application : exports false/true et seed42/42 vérifiés. Transcript : build/simple-hardened-demo/transcript.txt. Aucun benchmark complet, commit ou push.
