# Blockchain Security Lab

État : **0.6.0 — terminal PoW et M4 instrumenté**, détection statistique et défenses intégrées au simulateur PoW multi-nœuds à événements discrets, avec noyau M1
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
financières réelles. M5 reste la référence historique gelée. Le laboratoire est PoW uniquement ; aucun PoS distribué, backend ou frontend n’est prévu.

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
Les attaques ne sont pas combinées. PoS distribué, backend, frontend et ML sont exclus par décision du propriétaire.

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
sur plusieurs seeds. M5 a livré sa campagne historique ; les extensions courantes restent en C++ PoW sans web ni ML.


## Terminal interactif — version 0.5.0

```powershell
./build/ucrt64/blockchain_sim.exe --interactive
```

Mode simple : six paramètres seulement (nœuds, mineurs, hauteur, attaque, puissance adverse si pertinente, sécurité). Récapitulatif court puis lancement. Pendant le calcul : uniquement « Simulation en cours... ». Résultat en langage simple, verdict d’attaque, défense évaluable seulement selon un critère déclaré, et deux ou trois indicateurs utiles. Le menu « Comparer » exécute les deux variantes dans l’application, puis reste ouvert. Aucun détail technique dans ce parcours ; [critères et exports](docs/TERMINAL_SIMPLE.md).

Mode personnalisé : seed, poids individuels (zéro permis), rôles malveillants coordonnés, cible, topologie anneau/complète/ligne/manuelle, latences/jitter par lien manuel, confirmations marchandes, membres du pool ou identités Sybil/capture, difficulté, limites, détection fixe/adaptative et paramètres de politique utiles au scénario. Les groupes administratifs Sybil sont configurables mais supposés observables et fiables. `b`/`retour` revient à la question précédente, `q`/`annuler` quitte, Entrée garde la valeur proposée. `?`/`aide` montre la proposition et les bornes sans encombrer chaque question. Le récapitulatif avant lancement permet de choisir directement le numéro d’un paramètre à corriger. La stratégie de publication/retenue reste celle de M3 ; aucune stratégie arbitraire programmable n’est promise.

Les mineurs malveillants utilisent leurs propres poids ; le groupe n’ajoute aucun mineur virtuel. Ils partagent instantanément leur état privé dans ce modèle de coalition. Le terminal affiche blocs/parents, réceptions, forks, branches retenues `[PRIVE]`, publications, réorganisations, alertes et actions. Cette vue pédagogique n’alimente jamais les détecteurs. Aucune attente réelle n’influence le temps virtuel.

Chaque expérience crée un répertoire neuf `results/raw/interactive/run_N/`, avec `config.json` et `results.json`. Le menu permet une nouvelle expérience, un rejeu, ou une comparaison avec les défenses inversées. Les configurations sont rejouables par la CLI historique `--config FILE --output DIRECTORY`. Aucun JSON manuel n’est requis dans le terminal. La détection adaptative lance d’abord une calibration honnête séparée (seed dérivée et affichée), exporte sa référence et refuse une référence sans échantillons utilisables. Les caractéristiques insuffisamment calibrées sont ignorées et comptées, jamais inventées. Les chemins de référence diffèrent entre répertoires ; les observations simulées sont reproductibles, et le rejeu du JSON sauvegardé est identique pour la même construction.

`target_height` (0 = comportement historique) désigne la première hauteur atteinte chez `observer`. En 0.5.1, la production continue pendant `post_target_ticks` (0 = fermeture immédiate), puis les messages en file sont traités sous limites ; une réorganisation peut encore changer la hauteur finale. `max_blocks` reste le plafond global de découvertes, privées comprises. Une hauteur souhaitée n’est pas une garantie de progression.

Le résultat distingue `success`, `failed`, `inconclusive`, `baseline`. Une branche privée encore en attente, un paiement jamais accepté ou une limite temporelle/événementielle peut rendre l’issue non concluante. Zéro paiement accepté n’est jamais présenté comme une protection réussie. Le succès économique 51 % reste le renversement réel d’un paiement accepté, dans la vue finale du marchand. Ces résultats restent des observations finies du modèle.

## Politique marchande révisée

`security.hold_ticks` suspend les nouvelles acceptations. `confirmation_ticks` (défaut 1000, au moins hold_ticks) conserve indépendamment le seuil renforcé ; après la suspension le marchand peut réellement accepter sous ce seuil. L’augmentation vaut profondeur arrondie + confirmation_increment, plafonnée sans abaisser la règle initiale. Des alertes répétées renouvellent les durées ; des paiements honnêtes peuvent donc être retardés ou rester refusés. Les tests couvrent ces coûts.

`event_audits` déclenche des audits sur les forks/réorganisations publics locaux, sans accès aux blocs privés. `proactive_signals` (défaut false) autorise aussi les alertes de fréquence de forks/silence à modifier la politique : elles ne prouvent pas une attaque. Les audits périodiques restent configurables. Une réaction après acceptation ne sauve pas le paiement passé ; aucune garantie contre une majorité de hash power n’est donnée. Le consensus et les trois autres mitigations restent inchangés.

Les nouveaux scénarios fixent `attack.public_notice=true` : l’acceptation marchande est annoncée par messages P2P avec latence, puis l’adversaire compare sa chaîne privée à sa propre vue publique. Les anciens JSON sans ce champ gardent l’oracle M3 explicitement documenté. Le changement d’observation n’est pas une nouvelle stratégie destinée à favoriser la défense.

Schéma de résultats 2, modèle `pow_terminal_v2` : séquence de l’événement, détail des traces, confirmations observées/requises à l’acceptation, notification publique et issue explicite. Les résultats M5 restent au schéma 1 / moteur 0.4.0.

## Préservation M5 et validation séparée

```powershell
./scripts/preserve-m5.ps1
./scripts/test-ucrt64.ps1
./scripts/run-validation.ps1                  # prépare seulement
./scripts/run-validation.ps1 -Smoke -Run      # huit vérifications, pas une estimation statistique
```

L’archive locale ignorée `build/m5-reference/` conserve sources gelées, binaire et DLL ; ne pas la supprimer comme un build jetable. Le vérificateur historique M5 s’attache à cet arbre gelé ; il échouera normalement sur l’arbre courant modifié. Les écritures du moteur vers les espaces `m5_30seeds` et `m5-reference` sont refusées. Les rapports/configurations/manifeste/données M5 n’ont pas été remplacés. L’archive locale n’est pas une sauvegarde distante.

La nouvelle évaluation est décrite dans `analysis/POW_VALIDATION_V2_PROTOCOL.md`. Elle prépare 1680 évaluations et sept calibrations distinctes ; aucune campagne complète nouvelle n’est annoncée comme réalisée. `-Run` l’exécuterait séquentiellement dans un nouvel espace `pow_validation_v2`, sans toucher à M5. Les seeds 2001..2030 sont distinctes de M5 ; le quatrième mode honnête protégé mesure les fausses sanctions.

L’énoncé académique mentionne PoW/PoS. Le périmètre PoW uniquement est une décision explicite du propriétaire ; `exo4.cpp` reste la démonstration PoW/PoS d’Atelier 1, hors du laboratoire distribué. Aucun PoS distribué n’est revendiqué.


## Lisibilité et arrêt — 0.5.1

L’affichage synthétique est proposé par défaut : découvertes/producteurs, premiers forks/alertes et actions regroupés, publications privées, réorganisations et paiement accepté/renversé. Les occurrences répétées et les réceptions/envois P2P restent dans l’export complet. Choisir l’affichage détaillé pendant la configuration pour voir hashes, ticks, séquences, envois/réceptions et observations locales. Les deux affichages produisent des résultats JSON identiques pour une même configuration : le renderer ne lit que le moteur.

`post_target_ticks` continue la production normale après la première hauteur cible, pendant une durée définie avant le lancement. Le terminal propose 20 intervalles moyens, modifiables (0 = arrêt à la cible). Les plafonds de temps/découvertes/événements restent prioritaires. Aucune branche privée n’est publiée de force. L’issue et le motif d’arrêt/limite sont affichés séparément ; une réussite déjà observée peut coexister avec un plafond atteint. Une branche privée encore ouverte reste non concluante si l’objectif n’est pas atteint.

Les minuteries devenues inactives sont annulées sans faire avancer l’horloge. Les alertes exportent `phase` (production, drain, final_observation) et `operational` ; ce dernier indique si l’audit peut appliquer une action selon la configuration, pas si chaque alerte a effectivement déclenché une mitigation. Les alertes finales restent conservées, sans action. Les seuils des détecteurs et les trois autres mitigations ne sont pas modifiés.

Essais ciblés reproductibles, sans M5 :

```powershell
./scripts/test-ucrt64.ps1
./scripts/check-terminal.ps1
```

Le second script crée un espace neuf sous `results/raw/terminal_review/` et exécute 14 sessions CLI : quatre attaques sans/avec défense, quatre configurations personnalisées et deux contrôles à 85 %. Il enregistre saisies, stdout, chemins et empreintes des résultats. Ce n’est pas une campagne statistique. Voir [le rapport des essais et le diagnostic des 202 alertes](analysis/TERMINAL_REVIEW_051.md).


## Raffinement scientifique M4 — 0.6.0

Les stratégies M3 et les seuils fixes historiques sont conservés. Les nouvelles options sont explicites, sans activation automatique dans les anciens JSON :

- `security.max_payment_hold_ticks` : plafond de suspension pendant un épisode de risque (0 = renouvellement historique illimité). Les confirmations renforcées persistent indépendamment. Un plafond peut améliorer l’acceptation honnête et augmenter le risque adverse : aucun gain garanti.
- `stratify_reference` : moments honnêtes par nœud/composant/métrique ; `strict_reference` : exiger une référence liée au profil réseau. Les nouveaux exports de calibration portent le profil SHA-256 et la source honnête ; les références historiques restent lisibles en mode non strict. Strates absentes : abstention comptée, pas valeur inventée.
- `restrict_bootstrap` et `bootstrap_peers` : restreindre les nouveaux candidats publics ; `bootstrap_delay_ticks` : délai effectif avant connexion (défaut0). Aucun lien n’est activé à l’avance ; l’absence de candidat indépendant peut empêcher toute récupération.

`results.json.security.study` contient les décisions publiques marchandes (éligibilité, confirmations, suspension, séquence), les coûts de probation par émetteur et producteur, les crédits/payouts de pool, les tentatives de connexion et la première adoption observée. Les étiquettes honnête/adverse servent uniquement à l’évaluation après simulation. Une occasion de base sur la trajectoire protégée n’est pas un contre-factuel sans défense ; utiliser les contrôles appariés.

Exemples optionnels (répertoires neufs recommandés ; la CLI historique peut remplacer un résultat hors M5) :

```powershell
./build/ucrt64/blockchain_sim.exe --config configs/security_refinement/network_calibration.json --output results/raw/security_refinement/network
./build/ucrt64/blockchain_sim.exe --config configs/security_refinement/network_validation.json --output results/raw/security_refinement/network_validation
./build/ucrt64/blockchain_sim.exe --config configs/security_refinement/sybil_restricted.json --output results/raw/security_refinement/sybil_restricted
```

Résultats, coûts et limites : [rapport M4](analysis/M4_REFINEMENT_REPORT.md). Hypothèses écrites avant implémentation : [plan des lots](docs/SECURITY_REFINEMENT_PLAN.md). [Proposition de benchmark indépendant](analysis/M4_INDEPENDENT_BENCHMARK_PROPOSAL.md) **non lancée, soumise à validation**. La référence M5 et ses résultats restent préservés.


Renforcement optionnel M4 : `security.hardened_security=true`, exemples reproductibles sous `configs/final_defenses/`. Confirmations préventives, probation par producteur public, séquestre de shares restituable et diversité P2P préventive. Les anciens JSON et le terminal conservent leur politique précédente. Hypothèses et limites : `docs/FINAL_DEFENSE_PROTOCOL.md` et section `hardened_m4_v1` de `docs/SIMULATION_MODEL.md`. Les résultats ciblés sont documentés dans `docs/FINAL_DEFENSE_RESULTS.md` ; M5 reste inchangé.


Démonstration du terminal simple mis à jour : depuis la racine, `./build/ucrt64/final-bin/blockchain_sim.exe --interactive`. Ce binaire a été recompilé et exécuté ; l'ancien chemin build/ucrt64/blockchain_sim.exe n'est pas celui de cette démonstration. Fixe/adaptatif activent automatiquement hardened_security en simple, aucune défense le désactive. Comparer utilise false/true avec la même seed et des références honnêtes adaptées à chaque extracteur en mode adaptatif. Mode personnalisé inchangé.
