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
