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
