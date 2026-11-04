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
