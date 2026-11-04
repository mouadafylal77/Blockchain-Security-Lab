# Blockchain Security Lab

État : **M4**, détection statistique et défenses intégrées au simulateur PoW multi-nœuds à événements discrets, avec noyau M1
conservé. Configurations JSON, topologie, latences, minage probabiliste,
arbres locaux, forks, travail cumulé et traces de réorganisation sont exécutables.
`exo4.cpp` reste le code de référence Atelier 1, sans modification et hors compilation.

## Construire et vérifier sous Windows

Dépendances : GCC compatible C++17 **MSYS2 UCRT64**, OpenSSL Crypto UCRT64,
CMake >= 3.21, Ninja et CTest. Aucun besoin de Python ni de serveur web.
Installation MSYS2 habituelle, à effectuer dans un terminal UCRT64 si nécessaire :

```sh
pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-openssl mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja
```

Depuis PowerShell à la racine du dépôt :

```powershell
./scripts/test-ucrt64.ps1
```

Le script cherche CMake/Ninja dans UCRT64, puis le PATH, puis l'installation
Visual Studio 2022 BuildTools habituelle. Il affiche les chemins réellement employés.
CTest est recherché à côté de CMake. Les outils de construction natifs de Visual
Studio peuvent piloter GCC UCRT64 : le compilateur reste explicitement `g++.exe`,
sans MSVC. Le script ne télécharge ni n'installe de logiciel et restaure le PATH.
Il s'arrête avec une erreur si une dépendance ou une étape échoue.

Le preset suppose MSYS2 dans `C:/msys64/ucrt64`. Adapter le preset pour un autre
emplacement. Si les outils sont déjà dans le PATH, les étapes sont :

```powershell
$env:PATH = "C:\msys64\ucrt64\bin;$env:PATH"
cmake --preset ucrt64
cmake --build --preset ucrt64 --parallel 2
ctest --preset ucrt64 --output-on-failure
./build/ucrt64/blockchain_sim.exe --demo
```

Le preset de compilation limite aussi les tâches à deux. Les fichiers générés
restent dans `build/ucrt64/`, ignoré par Git. CTest conserve les 13 tests M1 et
ajoute les tests M2 décrits dans le journal. Les vérifications C++ utilisent des exceptions et
restent actives dans une compilation Release (`NDEBUG`).

## CLI M1

```text
blockchain_sim [--demo|--help]
```

Sans argument ou avec `--demo`, le programme crée le genesis et mine deux blocs
avec difficulté 2, aux instants simulés 1 et 2. Il affiche le scénario, l'unité de
temps, les indices, nonces, hashes et `chain_valid=true`. Aucun aléa ni temps réel
n'intervient ; une seed est donc sans objet à ce stade. Codes de sortie : 0 succès,
1 erreur de minage/validation, 2 arguments invalides.

Cette démo conserve les hashes et nonces M1. Aucun résultat expérimental
multi-nœuds n'est produit par `--demo`.

## CLI M2

```powershell
./build/ucrt64/blockchain_sim.exe --config configs/baseline.json --output results/raw/m2_baseline
./build/ucrt64/blockchain_sim.exe --config configs/fork_reorg.json --output results/raw/m2_fork
```

Le répertoire de sortie est créé si nécessaire ; `results.json` est remplacé
s'il existe. Les deux options sont obligatoires pour une simulation. Codes de
sortie : 0 exécution/export réussis (y compris arrêt normal par limite),
1 configuration/minage/capacité/écriture en erreur, 2 arguments CLI invalides.
Un résultat arrêté par limite peut contenir des messages en transit et des vues
divergentes : consulter `execution.stop_reason` et les compteurs finaux.

`baseline.json` : deux mineurs 60/40 et un observateur sans hash power, liens en
ligne à faible latence. `fork_reorg.json` : deux mineurs 50/50 et un observateur,
latences du même ordre que l'intervalle de découverte. Ces paramètres avec seed
42 produisent réellement des forks et des réorganisations dans les tests.
Ce sont des exemples de fonctionnement, pas une campagne statistique.

Configuration minimale : tous les champs de ces exemples sont obligatoires.
Les poids `hash_power` sont relatifs, indépendants du nombre de nœuds ; zéro
désactive le minage. Les liens sont bidirectionnels, avec délai par message
`latency_ticks + uniforme_entier(0, jitter_ticks)`. Les partitions sont autorisées.
Noms dupliqués, clés inconnues/dupliquées, mauvais types, valeurs hors bornes et
liens incohérents sont rejetés. Les bornes et le schéma de sortie sont détaillés
dans [SIMULATION_MODEL.md](docs/SIMULATION_MODEL.md).

JSON est lu/écrit avec l'unique en-tête versionné nlohmann/json 3.11.3
([provenance et licence](third_party/nlohmann/README.md)). Aucun framework de
tests ni téléchargement à la compilation. La reproductibilité porte sur le
JSON entier pour une configuration, une seed et une même construction ; les
variations de calcul flottant entre plateformes ne sont pas garanties identiques.

## Modèle et périmètre

Voir [le modèle du noyau](docs/SIMULATION_MODEL.md) et
[l'audit et le journal](docs/DEVELOPMENT_LOG.md). Réseau abstrait fiable, topologie
fixe et difficulté commune fixe : ni bande passante, ni pertes, ni transactions
financières réelles. Les campagnes statistiques M5,
PoS, backend et frontend sont différés.

## M3 : exécuter les attaques

```powershell
./build/ucrt64/blockchain_sim.exe --config configs/attack_51.json --output results/raw/attack_51
./build/ucrt64/blockchain_sim.exe --config configs/selfish_mining.json --output results/raw/selfish_mining
./build/ucrt64/blockchain_sim.exe --config configs/block_withholding.json --output results/raw/block_withholding
./build/ucrt64/blockchain_sim.exe --config configs/sybil.json --output results/raw/sybil
```

Chaque exemple a un fichier `_control.json` (même modèle, attaque désactivée) et
un `_failure.json` (condition d'échec contrôlée). Les résultats bruts incluent
l'objet `attack`, les paramètres, compteurs de rétention/publication, résultats
locaux et, pour la pool, shares, solutions complètes et payouts. Le modèle
scientifique détaille précisément les objectifs de succès et les simplifications.
Une seed favorable n'est pas une estimation d'efficacité ou de rentabilité.
Les attaques ne sont pas combinées. PoS, backend et frontend restent différés ; le ML est exclu.

## M4 : détection statistique et mitigation

Aucun Machine Learning. Détecteur fixe de référence et détecteur adaptatif à
fenêtres glissantes/EWMA, avec alertes explicables et vérité terrain séparée.
Les règles et formules figurent dans `docs/SIMULATION_MODEL.md`.

```powershell
./scripts/test-ucrt64.ps1
./scripts/run-m4-examples.ps1
```

Le second script exécute quatre calibrations honnêtes (seed 7), puis les 24
comparaisons : quatre attaques, deux détecteurs, modes baseline/attacked/protected
(seed 42). Il produit `results/raw/m4/comparison.json`. Les références sont générées
localement ; une exécution adaptative directe nécessite d'abord son fichier de
calibration. Aucun téléchargement ni dépendance supplémentaire.

Exécution manuelle, depuis la racine :

```powershell
./build/ucrt64/blockchain_sim.exe --config configs/m4/sybil_calibration.json --output results/raw/m4/sybil_calibration
./build/ucrt64/blockchain_sim.exe --config configs/m4/sybil_adaptive_protected.json --output results/raw/m4/sybil_adaptive_protected
```

Les défenses agissent sur les nouvelles acceptations marchandes, l'attente de
messages tardifs, le crédit de shares et les connexions/rattrapages P2P. Le choix
PoW par travail cumulé est conservé. Une alerte n'identifie pas avec certitude une
attaque. La protection 51 % peut arriver après le paiement ; une pool ne récupère
pas ses solutions retenues ; une probation peut pénaliser un relais honnête.
Les exemples conservent leurs faux positifs et ne démontrent aucun taux général
sur plusieurs seeds. M5, Python, PoS et web restent différés.
