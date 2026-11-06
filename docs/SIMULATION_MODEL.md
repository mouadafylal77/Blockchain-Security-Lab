# Modèle de Blockchain Security Lab

Les sections M1–M4 décrivent la référence historique 0.4.0 ; les sections 0.5.0 et 0.5.1 précisent les changements successifs. Les résultats M5 ne sont pas recalculés.

## Noyau M1 conservé

Ce noyau pédagogique n'émule pas Bitcoin ou Ethereum. Il reprend de l'Atelier 1
les concepts de hash SHA-256, Merkle root, nonce et liens entre blocs, avec une
validation indépendante de l'affichage et un genesis strictement défini.

- Une chaîne locale **linéaire** commence par un genesis fixe (indice 0, parent
  `0`, transaction `Bloc genesis`, temps/difficulté/nonce à 0). Le genesis est
  exempté de PoW ; tous ses champs et son hash sont vérifiés.
- Les transactions sont des chaînes opaques ordonnées, sans signature, solde,
  frais ou sémantique de paiement. Leur validation porte sur l'intégrité.
- Les feuilles Merkle sont les SHA-256 hexadécimaux des transactions. Les paires
  sont concaténées sous forme hexadécimale puis hachées ; un dernier élément
  impair est dupliqué. La racine vide est SHA-256 de la chaîne vide.
- Le hash d'un bloc engage, dans cet ordre, le domaine `BSL-PoW-v1`, l'indice,
  le parent, la racine Merkle, le **nombre de transactions**, le temps simulé,
  la difficulté et le nonce. Chaque champ est encodé `longueur:valeur`, avec
  longueur en octets et entiers décimaux sans zéro initial. Le nombre de
  transactions évite l'ambiguïté Merkle entre une liste impaire et cette même
  liste augmentée d'une copie de sa dernière transaction. Le hash stocké est
  exclu de son propre préimage. SHA-256 est fourni par OpenSSL.
- Le temps est un entier non signé en ticks arbitraires, fourni par l'appelant,
  croissant au sens large. Aucun temps machine n'entre dans les hashes.
- La PoW exige de 1 à 6 chiffres hexadécimaux `0` en tête du hash. La difficulté
  est un paramètre pédagogique par bloc, sans ajustement automatique. Le travail
  attendu est proportionnel à `16^d`. La classe linéaire M1 reste inchangée ;
  le nouveau `BlockTree` M2 décrit ci-dessous gère les branches séparément.
- Le minage essaie successivement les nonces depuis zéro, avec budget explicite
  d'un million d'essais par défaut. L'épuisement produit une erreur ; le budget
  ne garantit pas de trouver un bloc, notamment aux fortes difficultés. Le
  minage effectif M1 sera distinct du processus stochastique simulé de M2.
- Un ajout vérifie la PoW, les engagements, l'indice, le parent et le temps avant
  de modifier la chaîne. Aucun mécanisme de consensus distribué n'est présent.

La reproductibilité de la démo M1 est une égalité des hashes/nonces pour des
entrées identiques, sans RNG. Le PoS de l'Atelier 1 reste uniquement dans
`exo4.cpp` ; son périmètre académique nécessite une clarification avant les
scénarios d'attaques. Aucun résultat PoW n'est extrapolé au PoS.

## M2 : événements, minage et temps

La file est ordonnée par `(timestamp entier, numéro de séquence croissant)`.
La séquence est attribuée à l'insertion, même si plusieurs événements ont le
même timestamp ; un événement inséré pendant un traitement passe après les
événements déjà planifiés à cet instant. Aucun sommeil ni temps machine n'est
utilisé. Le dernier tick traité est exporté ; ce n'est pas une durée CPU.

Chaque mineur i de poids positif h_i possède un processus de découverte de
taux `lambda_i = (h_i / somme(h)) / mean_block_ticks`. Un poids nul ne crée
aucun événement de minage. Le nombre d'identités n'ajoute donc aucun hash power.
L'intervalle global visé est `mean_block_ticks` à la difficulté commune configurée.
En unités de travail, cela calibre le hash power total à
`16^difficulty / mean_block_ticks` ; changer difficulté en gardant cet intervalle
constant recalibre ce total. Ce n'est pas un hashrate matériel mesuré.

Les attentes exponentielles sont calculées par inversion `ceil(-log(U)/lambda)`,
au minimum un tick. `U` est dans l'intervalle ouvert (0,1), dérivé des 52 bits
supérieurs de `std::mt19937_64`. La discrétisation ajoute un biais de résolution :
choisir des ticks suffisamment fins relativement aux intervalles et latences.
Les attentes au-delà de 10^15 ticks sont plafonnées à cette valeur, toujours
au-delà de l'horizon maximal autorisé (10^12). Les ratios de poids qui rendent
le taux non représentable sont rejetés.

Un événement de découverte utilise **la tête locale au moment où il est traité**.
Une réception changeant la tête ne replanifie pas la prochaine découverte :
les taux sont constants et la loi exponentielle est sans mémoire. Le mineur
travaille implicitement sur la tête courante après chaque réception. Après sa
propre découverte, il tire une nouvelle attente. Les événements simultanés
suivent la règle de séquence, sans simultanéité physique prétendue.

À chaque découverte, le mineur construit un bloc puis appelle le minage M1 borné
pour obtenir une preuve SHA-256 réellement vérifiable. Ce coût CPU et le nombre
de nonces essayés **ne consomment aucun tick simulé** et ne déterminent pas les
taux de découverte. L'échec du budget PoW interrompt l'exécution avec erreur,
sans publier de résultat complet. Les transactions synthétiques engagent l'id
du mineur et l'ordre global de découverte ; aucun solde ni signature n'est simulé.

Le RNG de minage utilise la seed de configuration ; le RNG réseau utilise
`seed XOR 0x9e3779b97f4a7c15`. Le jitter ne perturbe donc pas la suite des tirages
de minage. Les tableaux de nœuds déterminent leur ordre d'initialisation ; les
voisins sont triés par indice. Mêmes entrées et même construction produisent
le même JSON, sans métadonnées dépendantes du temps réel ou du chemin de sortie.
Le `log` flottant interdit de garantir une identité bit à bit entre compilateurs
ou bibliothèques mathématiques différents.

## Réseau et vues locales

Les liens sont bidirectionnels, fixes, sans bande passante ni perte. Chaque
message reçoit indépendamment une latence fixe plus un jitter entier uniforme
inclusif. Zéro tick est permis. Les partitions sont autorisées ; leur convergence
n'est pas garantie. Il n'existe pas de synchronisation globale forcée en fin de run.

Chaque nœud possède son propre `BlockTree` et sa propre mémoire des arrivées.
Le registre du simulateur partage uniquement des contenus de blocs immuables
et leur origine, jamais une chaîne mutable. Un bloc connecté est envoyé une
fois à chaque voisin, y compris au voisin émetteur ; les retours et les chemins
multiples produisent des doublons comptés, sans relayage supplémentaire.

Un bloc dont le parent manque est validé intrinsèquement puis mis en attente,
sans devenir actif ni être relayé. Quand son parent arrive, les descendants
valides sont rattachés de façon déterministe (ordre des hashes), puis relayés.
Un descendant dont l'indice ou le temps est incompatible avec son parent est
retiré de l'attente et rejeté. Les autres parents manquants restent en attente
jusqu'à leur réception ou la fin du run. Il n'y a pas encore de requête de parent,
d'expiration, de bannissement ou de modèle d'injection malveillante.

## Travail cumulé, forks et réorganisations

Le genesis commun a un travail nul. Chaque bloc valide contribue exactement
`16^d` unités sous ce modèle de préfixe hexadécimal ; la somme est un uint64
avec vérification de dépassement. La meilleure tête maximise cette somme.
À travail égal, le hash de tête lexicographiquement le plus petit gagne,
indépendamment de l'ordre de réception final. Ce choix pédagogique ne reproduit
pas le choix réel de Bitcoin et influence les courses entre branches.

Le simulateur mine à difficulté commune fixe, sans ajustement. Le `BlockTree`
accepte les difficultés 1..6 autorisées par M1 et permet de tester qu'une branche
plus courte mais plus coûteuse gagne ; aucune politique de difficulté contextuelle
n'est revendiquée. Une majorité de nœuds ne participe jamais au choix de tête.

Un fork observé est l'ajout local d'un deuxième enfant (ou suivant) à un parent
déjà connecté. Une réorganisation est un changement de tête retirant au moins
un bloc actif ; sa profondeur est le nombre de blocs détachés jusqu'au dernier
ancêtre commun. Une extension simple n'est pas une réorganisation. Les changements
de tête par égalité de travail sont aussi comptés lorsqu'ils détachent un bloc.
Chaque attachement d'un descendant en attente peut produire sa propre transition.
Les confirmations locales d'un bloc actif de hauteur h seraient `tip_height-h+1`
(inclusion comprise) ; aucune politique marchande n'est implémentée en M2.

**Stale** : bloc connecté, valide, hors de la chaîne active de ce nœud.
**Parent inconnu / orphan** : bloc reçu mais non connecté faute d'ascendance.
**Invalide** : échec de PoW/engagements/genesis/continuité ; il ne devient pas stale.
La classification est locale et peut changer après une réception ou réorganisation.

## Configuration JSON et bornes (schéma M2)

Tous les champs de `configs/baseline.json` sont obligatoires, sans valeurs
implicites lors de la lecture JSON. Les clés inconnues et dupliquées sont rejetées.
Entiers, booléens, nombres flottants et chaînes ne sont pas interchangeables.

| Champ | Domaine |
|---|---|
| scenario | chaîne non vide, au plus 128 octets |
| seed | entier non signé 64 bits |
| duration_ticks | entier 1..10^12 |
| max_events / max_pending_events | entiers 1..10^6 |
| max_blocks | entier 1..10000, hors genesis |
| difficulty | entier 1..6 |
| mining_attempts | entier 1..10^7, budget par bloc |
| mean_block_ticks | nombre fini 1..10^9 |
| nodes | 1..256 objets `{id, hash_power}` |
| id | chaîne unique non vide, au plus 64 octets |
| hash_power | nombre fini 0..10^12, somme positive, taux représentable |
| links | objets `{from, to, latency_ticks, jitter_ticks}` ; ids existants, pas de boucle ou doublon non orienté |
| latency_ticks / jitter_ticks | entiers 0..10^9 |

Le fichier est limité à 1 MiB et la profondeur JSON à 16. Les exemples restent
volontairement petits pour la machine de développement. Les bornes maximales
ne constituent pas une garantie de performance pour toutes leurs combinaisons.

`max_blocks` ferme le minage ; les événements de minage restants deviennent
inactifs et les livraisons sont traitées tant que les autres limites le permettent.
L'horizon ou `max_events` peuvent arrêter avant la vidange. `max_pending_events`
est une limite dure : dépassement = erreur, pas perte silencieuse de messages.
Chaque arbre local peut mémoriser au plus `max_blocks+1` blocs, genesis et attente
compris. Une capacité atteinte ou un dépassement arithmétique produit une erreur.

## Résultat JSON (schema_version = 1)

Un fichier `results.json`, UTF-8, contient :

- `software_version`, `build` (compilateur, standard C++), `model` et `config`
  intégrale normalisée, seed comprise. Les chemins et l'heure réelle sont exclus.
- `execution` : cause d'arrêt, dernier tick traité, événements restant en file.
  `max_blocks_drained` signifie minage fermé et file vidée ; `duration_ticks`
  ou `max_events` signalent des observations potentiellement censurées.
- `metrics` : événements, productions, messages envoyés/livrés, doublons,
  arrivées à parent inconnu, observations de fork, réorganisations et propagation.
- `nodes` : id, tête, hauteur, travail cumulé, hashes de la chaîne active (genesis
  compris), blocs connus hors genesis, stale, en attente, produits par ce mineur
  et ses propres blocs actifs dans sa vue finale.
- `blocks` : tous les blocs produits hors genesis, triés par hash, avec parent,
  hauteur, mineur, date simulée de découverte, difficulté, nonce, transactions
  et racine Merkle. Le timestamp du bloc est cette date de découverte.
- `trace` : découvertes, extensions, forks et réorganisations dans l'ordre de
  traitement ; tick, nœud, hash, anciennes/nouvelles têtes et profondeur détachée.
  Les champs de tête sans objet sont des chaînes vides ; profondeur 0 sinon.
- `measurement_definitions` : définitions courtes intégrées au résultat.

Les forks et réorganisations sont des **observations par nœud**, pas des forks
uniques globaux. La propagation mesure la première arrivée du contenu à chaque
nœud hors origine moins la découverte, même si le parent manque encore ; ce
n'est pas un temps de validation. Moyenne/max en ticks, nombre d'échantillons
fourni ; valeur 0 par convention si aucun échantillon. Les nœuds inaccessibles
et les arrivées après l'arrêt n'apportent pas d'échantillon. Une moyenne seule
ne démontre donc pas une bonne connectivité.

La distribution des blocs produits inclut les stale, sans récompense économique.
`own_active_blocks` utilise la vue finale de chaque mineur ; en cas de partition,
ce n'est pas un bilan de récompenses global. Les coûts CPU, allocations mémoire,
temps de détection, pertes et récompenses financières ne sont pas mesurés en M2.

## Décisions scientifiques à valider avant les expériences d'attaque

M2 utilise des choix d'ingénierie documentés pour être exécutable. Avant de figer
le protocole M3–M5, valider : résolution temporelle et calibration du hash power,
départage lexicographique (qui influence le selfish mining), politique de difficulté,
topologie/latences/pertes pertinentes, et périmètre académique PoS. Les pools,
paiements/double dépense, récompenses, adversaires, détecteurs et mitigations
restent hors de ce jalon. Aucun résultat de défense n'est déduit de ces exemples.

## M3 : scénarios d'attaque

Les quatre modèles sont intégrés au même moteur événementiel et au fork choice
M2. `exo4.cpp` et les modules du noyau restent inchangés. Aucun détecteur ni
contre-mesure M4 n'est ajouté. Chaque configuration fournit un objet `attack` :
`type` (`51`, `selfish`, `withholding`, `sybil`), `adversary` et `target` (indices
zéro des nœuds), `confirmations` (1..10000). Options : `enabled` (défaut true),
`pool_members` et `sybil_identities` (listes d'indices uniques),
`capture_connections` (défaut true). Les membres et identités sont validés.
Sans objet attack, M2 reste honnête. Les paramètres normalisés sont exportés.

### Double dépense / 51 %

Un unique jeton pédagogique `coin` préexiste au genesis. La première inclusion
publique est `spend:coin:merchant`. L'adversaire mine depuis le genesis une branche
privée contenant `spend:coin:attacker`. Les générateurs ne réincluent aucune dépense
si l'ascendance en contient déjà une ; ce n'est pas un moteur UTXO générique avec
signatures ou balances. La confirmation inclut le bloc du paiement lui-même.
L'acceptation du marchand est mémorisée même si son paiement est ensuite retiré.
Publication parent d'abord uniquement après acceptation et si la somme des travaux
privés dépasse le travail de la vue du marchand. Après publication, l'adversaire
redevient mineur public. Le modèle donne connaissance instantanée des confirmations
du marchand à l'adversaire (oracle explicite, sans message de vente simulé).

Succès : paiement accepté puis dépense concurrente active dans la vue finale du
marchand, sans paiement marchand actif. Cela nécessite une réorganisation réelle.
La fraction adverse n'est pas testée contre un seuil magique de 51 % : un budget
fini et la seed influencent l'issue. La configuration d'exemple utilise 85 % pour
un cas pédagogique favorable ; un seuil de confirmations inaccessible échoue
malgré du travail privé. Un adversaire sans puissance échoue aussi.

### Selfish mining

Le mineur conserve une branche privée sur sa tête publique locale courante.
Il poursuit sa tête privée tant qu'elle existe. Une modification de sa vue
publique par concurrence déclenche : publication de toute la branche si son
travail est au moins égal au travail public ; abandon sinon. Une égalité crée
une course départagée par la règle de hash M2, les délais et les découvertes
suivantes. La branche non publiée à la limite d'exécution n'est pas vidée de force.
L'avance maximale observée et les blocs abandonnés/publiés sont exportés.

Il s'agit d'une stratégie simplifiée de rétention/publication, **pas de la machine
à états Eyal–Sirer complète** (pas de publication partielle aux avances élevées,
ni de paramètre gamma imposé). Succès du run : part des blocs actifs de l'adversaire
dans la vue cible supérieure à sa part de hash power, avec publication effective.
Cette observation sur une seed ne prouve pas un gain moyen ; comparer les contrôles
et plusieurs seeds avant toute conclusion de rentabilité.

### Block withholding et pool

`pool_members` désigne les participants ; l'adversaire doit en faire partie.
La difficulté réseau est au moins 2. Les blocs complets gardent une PoW réelle
M1 à difficulté d. La difficulté de share est d-1 : le modèle échantillonne les
shares non complètes à taux 15 fois le taux des blocs complets, avec un RNG
séparé (`seed XOR 0xd1b54a32d192ed03`). Chaque découverte complète constitue aussi
une share. Les shares sont des tickets probabilistes comptables : aucun hash
bas seuil supplémentaire n'est calculé. Leurs délais sont arrondis en ticks
comme les autres processus ; le ratio empirique peut donc différer de 16.

L'adversaire soumet sa share mais ne publie jamais sa solution complète ; les
autres membres publient normalement. Les mineurs utilisent la tête locale connue,
sans serveur de jobs explicite ni latence d'envoi des shares. La pool partage
une unité de récompense par bloc de ses membres actif dans la vue cible finale,
au prorata des shares cumulées sur le run (pas de rounds, frais ou PPS réel).
Les payouts doivent sommer à cette récompense, à l'arrondi flottant près.

Succès comportemental : au moins une solution complète retenue. Les tests exigent
aussi une perte de récompenses actives face au contrôle apparié sans rétention.
Le nombre de solutions retenues n'est pas présenté comme une perte économique
contre-factuelle exacte ; les branches et leurs stale peuvent changer. Un mineur
adverse de poids zéro échoue tout en laissant la pool honnête fonctionner.

### Sybil / isolement ciblé

`sybil_identities` désigne des nœuds existants contrôlés, distincts de l'adversaire
et de la cible, tous de hash power zéro. Leur nombre est le budget d'identités.
La cible est passive (poids zéro). Les liens d'accès des Sybil au réseau honnête
sont ceux du JSON. Avec `capture_connections=true`, tous les liens de la cible
sont remplacés par des liens vers ces identités (un tick chacun). Ces relais
reçoivent les blocs mais filtrent leurs envois vers la victime ; les autres
échanges suivent M2. La topologie effective est exportée séparément de l'entrée.

Succès : messages effectivement filtrés et victime restant au genesis à l'arrêt.
Avec `capture_connections=false`, les liens honnêtes de la victime subsistent ;
l'exemple démontre l'échec de l'isolement total. Ce modèle suppose la capture des
connexions au démarrage ; il n'établit pas qu'une création d'identités suffit à
capturer un vrai réseau. Pas de découverte dynamique de pairs, coût d'admission,
churn ou ajout automatique de hash power. La modification réseau ne change aucun
poids de mineur.

### Sorties et limites communes

`results.json.attack` ajoute : type, activation, succès selon les définitions
ci-dessus, acceptation marchande, compteurs retenus/publiés/abandonnés, avance
privée maximale, hashes encore privés, messages filtrés, blocs actifs adverses et
leur part dans la vue cible, liens effectifs. Pour la pool : difficultés distinctes,
shares, solutions soumises/retenues, récompenses actives et payouts par participant.
Les traces ajoutent `withheld` et `publication`. La liste globale des blocs inclut
les découvertes privées : la connaissance globale de l'export n'est pas donnée
aux nœuds. Les délais de propagation des blocs publiés incluent la rétention.

Une seule stratégie adverse par exécution ; pas de combinaison d'attaques.
L'arrêt peut censurer une branche privée ou des messages. Aucune précision/rappel,
détection, mitigation, perte monétaire réelle ou sûreté d'un protocole réel n'est
revendiquée. Les questions PoS et de calibration du modèle M2 restent ouvertes.

## M4 — Détection statistique et défenses opérationnelles

Choix du propriétaire : règles statistiques uniquement, **aucun Machine Learning**.
Les classes `Detector` et `FeatureExtractor` ne prennent ni simulateur, ni attaque,
ni vérité terrain en entrée. `Policy` reçoit uniquement les alertes et la
configuration de défense. Le moteur traduit ses événements publics en télémétrie.
Les champs privés de M3 restent disponibles pour l'évaluation finale seulement.

### Observations et fenêtres

Audit toutes les `audit_ticks` (100 par défaut), à partir du premier audit.
Les fenêtres sont `(t-window_ticks,t]` (2000 ticks par défaut), sans accès à des
échantillons futurs. Un dernier audit non opérationnel peut décrire la fin du run ;
il ne répare pas un réseau après l'arrêt. Les audits et remises différées consomment
le budget événementiel. Les autres limites M2 restent applicables.

| Caractéristique observable | Définition | Seuil fixe par défaut |
|---|---|---:|
| reorg_depth | Profondeur maximale détachée dans la fenêtre locale | 0,5 bloc |
| fork_rate | Nombre de nouveaux enfants concurrents / durée observée, limitée à la fenêtre | 0,005 / tick |
| late_p95 | Quantile empirique de rang plafond(0,95 n), par voisin et première réception du contenu | 40 ticks |
| stale_ratio | Blocs locaux connectés hors chaîne / blocs connus hors genesis | 0,25 |
| reward_concentration | Part maximale d'un producteur annoncé dans la chaîne locale active | 1,1 (descriptif, pas d'alerte par défaut) |
| share_deficit | Déficit de solutions complètes soumises par membre, décrit ci-dessous | 3 |
| peer_concentration | Taille du plus grand groupe de pairs / degré, si degré >= 2 | 0,8 |
| silence_ticks | Temps depuis le dernier message reçu, si au moins un voisin | 500 ticks |

`late_p95` utilise l'heure annoncée dans l'en-tête, **pas** le registre privé de
découverte ; un horodatage malveillant peut donc l'éluder. Les contenus déjà connus
ou déjà observés ne fournissent pas une nouvelle mesure de retard. Les messages
répétés prouvent seulement un contact pour `silence_ticks`, pas la fraîcheur de la
chaîne. Les parts de producteur reposent sur les identités annoncées du modèle M3,
sans authentification cryptographique ajoutée. Stale et concentration des récompenses
sont des états locaux courants ; les événements de forks/reorg et retards utilisent
la fenêtre glissante. Les moyennes/max de propagation globales M2 sont conservées.

Le registre public de pool distingue shares observées, shares créditées et solutions
complètes **soumises** par membre. Une découverte privée n'est jamais présentée au
détecteur comme une solution complète trouvée. Pour N shares observées et B solutions
complètes soumises dans la fenêtre, `E=N/16` et
`share_deficit=max(0,(E-B)/sqrt(max(1,E)))`. Aucun score avant `min_shares=64`.
Le dénominateur de type Poisson est une approximation conservatrice par rapport à
la variance binomiale Np(1-p), p=1/16. L'arrondi temporel M3 déforme le ratio effectif ;
ni ce score ni le chiffre 3 ne constituent une preuve ou un niveau de confiance garanti.
La fenêtre pool des exemples est 10000 ticks : avec 2000 ticks, le test constate
une attaque active non détectée, faute d'accumulation suffisante de shares.

### Référence honnête et calculs adaptatifs

Modes `security.mode` :

- `off` : comportement M1–M3, aucune détection/mitigation ni évaluation M4.
- `calibration` : exécution honnête, accumulation réelle de moments, aucune alerte ;
  écrit `calibration.json`. Une attaque active est rejetée dans ce mode.
- `baseline` : exécution honnête **évaluée**, avec détecteur actif, sans défense.
- `attacked` : détecteur actif sans défense, attaque selon le JSON.
- `protected` : mêmes observations/détecteur, application des défenses activées.

La référence adaptative est un fichier externe produit par une exécution de
calibration antérieure. Le constructeur rejette fichier absent, fenêtres
incompatibles, moments non finis/négatifs et nombre de mesures inférieur à
`min_samples=5`. Une métrique absente est ignorée avec compteur explicite, sans
inventer un seuil appris. Le fichier inclut les paramètres de la source et sa
version logicielle ; les moments initiaux chargés sont recopiés dans le résultat.
La provenance est déclarative : le fichier est une entrée scientifique de confiance.

Les moments par métrique (agrégés entre observateurs de la calibration) sont
calculés par Welford : `delta=x-mu`, `mu'=mu+delta/n`,
`M2'=M2+delta*(x-mu')`, variance de population `v=M2/n`.
Ce sont des observations de fenêtres chevauchantes, **pas** n essais indépendants.
Les exemples calibrent sur seed 7 ; les comparaisons honnête/attaquée/protégée
utilisent toutes seed 42. Aucun résultat attaqué ou protégé n'entraîne la référence.

Pour chaque flux (observateur, composant, métrique), initialisé aux moments honnêtes :

```text
B = seuil fixe de la métrique ; s_min = 0,1 B
s = sqrt(max(v, s_min^2))
T = max(B, mu + k s)                 # calculé AVANT l'échantillon courant
alerte si x > T                     # égalité acceptée
```

Une valeur dépassant T ne met jamais à jour l'EWMA, même si l'alerte est supprimée
par le cooldown. Sinon :

```text
x_c = clamp(x, mu-clip*s, mu+clip*s)
d = x_c-mu
mu' = clamp(mu+alpha*d, 0, mu_0+drift_cap*s_0)
v' = min((1-alpha)*(v+alpha*d^2), 4*s_0^2)
s_0 = sqrt(max(v_0, s_min^2))
```

Défauts : `alpha=0,1`, `k=3`, `clip=2`, `drift_cap=2`,
`cooldown_ticks=500` par flux. Le plancher gère la dispersion nulle et l'ancrage
limite une contamination progressive du seuil. Un changement normal hors de
cette bande peut produire des faux positifs et nécessite une nouvelle calibration
honnête explicite ; aucune mise à jour rétroactive n'est faite. Les seuils sont
un point de départ d'ingénierie configurable, **pas une calibration optimisée**.
Aucune hypothèse gaussienne justifiant un taux de faux positifs à 3 sigma n'est
revendiquée. `k`, `alpha`, limites, fenêtres et tous les seuils sont dans le JSON.

Les paramètres numériques doivent être finis et positifs ; alpha <=1, k <=20,
clip et drift_cap <=10, fenêtres/cooldown <=10^9 ticks, min_samples 2..100000,
min_shares 1..10^6. Les seuils sont plafonnés à 10^15 pour garder les moments
finis. Les entiers, booléens et chaînes ne sont pas interchangeables.

### Alertes et unité d'évaluation

Une alerte indique : instant simulé, observateur, composant visible (nœud, pair ou
participant de pool), famille de comportement, métrique, valeur, seuil réellement
appliqué et détecteur. Ce n'est pas une identification certaine d'une attaque.
Les familles sont : `chain_instability`, `publication_anomaly`, `pool_deficit`,
`connectivity_anomaly`. Les alertes tardives, de reconnection/catch-up ou dues à des
forks honnêtes sont conservées, pas retirées pour améliorer les scores.

Unité d'évaluation : **(exécution, famille)**, soit quatre unités par run évalué.
La vérité terrain assigne une famille principale à l'attaque configurée active
(51 %, selfish, withholding, Sybil respectivement), aucune en baseline. L'exposition
est l'épisode entier à partir de t=0, même si la tentative échoue ou manque de
ressources. Une ou plusieurs alertes de cette famille prédisent une unité positive.
Les alertes des autres familles sont des FP selon cette convention ; cette mesure
ne prétend pas étiqueter la causalité réelle de chaque fork ou retard. Une attaque
51 % peut légitimement produire aussi une anomalie de publication ; cette dernière
reste FP de classement primaire dans ce protocole, et non une observation fausse.

TP/FP/TN/FN sont calculés dans `metrics/detection_evaluation`, **après** le run.
`precision=TP/(TP+FP)`, `recall=TP/(TP+FN)`, `F1=2PR/(P+R)` ; zéro si dénominateur
nul. Les compteurs restent disponibles pour distinguer absence de cas et score nul.
Latence : premier tick d'alerte de la famille exposée moins 0 ; null si absente.
C'est une latence depuis le début de l'exposition, pas depuis une découverte privée
inobservable ou depuis la première preuve. Aucune information de vérité terrain
n'entre dans les caractéristiques, les EWMA ou les décisions opérationnelles.

### Contre-mesures

Les commutateurs `mitigate_confirmations`, `mitigate_publication`, `mitigate_pool`,
`mitigate_peers` sont indépendants, activés seulement en mode protected. Les exemples
isolent la contre-mesure correspondante, par configuration explicite. Le détecteur
et la politique ne consultent jamais le type d'attaque pour décider d'une action.

- Instabilité de chaîne : suspension de **nouvelles** acceptations durant
  `hold_ticks=500`, et confirmations élevées de `ceil(profondeur)+3`, plafonnées à
  `max_confirmations=100` sans abaisser la règle initiale. À expiration, retour à
  la règle initiale. Aucune vente déjà acceptée n'est annulée, aucun bloc PoW valide
  n'est refusé, aucun checkpoint n'est ajouté. L'effet réel sur l'instant
  d'acceptation est testé. L'exemple M3 de majorité reste réussi car le signal est
  trop tardif ; les confirmations ne suppriment pas une majorité de hash power.
- Publication anormalement tardive : probation du voisin observé pendant
  hold_ticks ; les nouveaux contenus reçus de ce voisin sont mis en file pendant
  `publication_delay` (20 par défaut, 100 dans l'exemple selfish). Une remise
  différée n'est pas mise en attente une deuxième fois. Les règles de validité,
  travail cumulé et départage restent inchangées. Ce délai expérimental affecte
  propagation et minage local ; il peut aussi nuire à un relais honnête lent et
  n'empêche pas une publication par un autre chemin. Le choix de 100 ticks
  (cinq intervalles moyens dans l'exemple) démontre un effet, sans être optimisé.
- Déficit de pool : suspension prospective du **crédit** de shares pendant hold_ticks.
  Les shares reçues restent observées pour l'audit, les crédits anciens restent
  acquis, les solutions complètes soumises et blocs valides restent acceptés.
  Les payouts M3 sont recalculés sur les crédits effectifs. Cela peut réduire la
  rémunération d'un participant suspect ; cela ne restaure pas les blocs retenus
  et peut pénaliser un mineur honnête malchanceux. Aucun seuil ne prouve une fraude.
- Anomalie de connectivité : limitation locale `max_peers=4`,
  `max_per_group=1`, conservation des voisins admissibles puis connexion à des
  groupes distincts d'un annuaire bootstrap public dans l'ordre déterministe.
  L'attribut optionnel `nodes[].peer_group` représente un groupe administratif
  observable (préfixe/organisation), jamais le label Sybil ; par défaut l'id.
  Il est supposé fourni par un mécanisme d'admission fiable. Un groupe distinct
  n'est pas une garantie d'honnêteté et les déclarations falsifiables affaiblissent
  cette défense. Les nouvelles connexions utilisent un tick ; les deux extrémités
  échangent leurs **chaînes actives locales connues**, sans registre omniscient.
  Les messages déjà en transit restent livrables. Les quotas s'appliquent à la
  sélection locale du nœud réagissant, pas à une optimisation globale de topologie.

Aucune contre-mesure ne change la sélection PoW par travail cumulé. Toutes peuvent
modifier les observations et les trajectoires de minage par leur effet réseau ou
commercial. Les attaquants M3 ne s'adaptent pas à la défense en changeant de stratégie.

### Sorties, coûts et comparaisons

`results.json.security` contient alertes, paramètres (dans config.security), moments
initiaux importés, voisins finaux, confusion, P/R/F1, latence, coûts et unités
économiques. Un jeton est perdu dans le modèle 51 % lorsqu'un paiement accepté est
renversé ; une unité de pool correspond à un bloc actif de membre dans la vue cible.
Le contre-factuel de perte de pool vient du contrôle apparié, pas d'une estimation
magique des solutions cachées. Les payouts et parts actives M3 sont conservés.

Coûts : messages totaux M2, événements traités, audits, caractéristiques évaluées,
nombre maximal d'échantillons conservés, reports de messages et somme de leurs
délais, reconnexions, messages de rattrapage et shares non créditées. Les échantillons
sont des unités logiques de stockage, **pas** une mesure RSS en octets. Les ticks
ne sont pas du temps CPU ; CPU réel/RSS ne sont pas mesurés. Les durées CTest sont
seulement des durées réelles de vérification, pas un surcoût simulé de défense.

`configs/m4/` fournit 4 calibrations et 24 cas comparatifs. Le script
`scripts/run-m4-examples.ps1` exécute ces exemples fixes et écrit
`results/raw/m4/comparison.json`. Ce n'est pas une campagne multi-seeds M5.
Les FP, FN, limites de détection et défenses insuffisantes font partie des résultats.


## 0.5.0 — Terminal PoW et politique marchande

Le propriétaire retient PoW uniquement malgré la mention PoW/PoS de l’énoncé. Aucun protocole PoS distribué, web ou ML n’est prévu. M1/M2 conservent leurs règles de validation et de travail cumulé.

Le générateur simple attribue aux seuls mineurs désignés des poids entiers uniformes 10..100 avec le RNG indépendant seed XOR 0x94d049bb133111eb. Le mode personnalisé remplace ces poids et permet une coalition coordonnée (adversaries, vide = adversaire unique historique). La coalition partage instantanément sa branche privée, sans coût réseau interne ; ses ressources restent la somme des poids des véritables mineurs. Les identités Sybil restent sans puissance et leur capture initiale demeure supposée. Le terminal ne choisit jamais une seed en fonction du résultat.

La hauteur cible est un seuil local chez observer, distinct du plafond de découvertes. Lorsqu’elle est observée, la production ferme puis la file est drainée sous les limites du run. Les branches privées ne sont pas publiées de force. Une vue finale peut redescendre après une réorganisation. Cette règle d’arrêt conditionnelle n’est pas utilisée par le nouveau protocole d’évaluation à budget fixe.

La politique 51 % sépare hold_ticks et confirmation_ticks : à t alerte, paiements suspendus jusqu’à t+hold ; confirmations renforcées jusqu’à t+confirmation_ticks. Le seuil vaut max(base,min(max_confirmations,base+extra)), extra=ceil(profondeur)+increment pour une réorganisation, increment seul pour les signaux précoces optionnels. Le maximum des renforcements encore actifs est conservé ; une alerte après expiration ouvre une nouvelle période. Une alerte répétée peut empêcher toute acceptation : ce coût de disponibilité est explicitement non concluant pour la défense.

Les audits événementiels optionnels examinent seulement les forks/réorganisations locaux déjà reçus. Ils peuvent réduire le délai d’audit, mais pas observer une chaîne privée. Plusieurs événements au même tick ne provoquent pas plusieurs mises à jour statistiques d’une même série à ce tick. Les signaux optionnels fork_rate/silence_ticks peuvent déclencher un hold avant une acceptation ; ils sont aussi possibles honnêtement. Pas de changement du fork choice, de checkpoint ou de rejet d’une PoW valide.

Les nouvelles expériences utilisent public_notice=true : une annonce marchande authentifiée au sens du modèle (pas une signature cryptographique ni un protocole commercial réel) est propagée par les liens P2P. L’adversaire attend sa réception puis compare le travail privé à sa vue publique locale. Le marchand conserve sa décision dans son propre contexte. public_notice=false reste le comportement oracle historique des JSON M3/M4. La rétention et la stratégie générale publication-après-acceptation ne sont pas réoptimisées. Les signaux de vente et les identités annoncées ne modélisent ni portefeuille ni signatures réelles.

Le schéma de sortie 2 ajoute outcome, détail et séquence des traces, événement payment_accepted et confirmations effectivement observées/requises. La vue pédagogique de découvertes privées est strictement séparée des entrées du détecteur. Le résultat success reprend l’objectif M3 ; failed signifie objectif non atteint sans état privé/acceptation/censure non résolus ; inconclusive couvre notamment paiement non accepté, état privé restant et limites dures. Ces catégories n’estiment pas une probabilité infinie de succès. Les quatre familles et l’unité d’évaluation M4 restent déclarées ; un avertissement précoce de connectivité n’est pas automatiquement un TP d’identification 51 %.

Le terminal adaptatif produit une calibration honnête préalable avec seed XOR 0xd6e8feb86659fd93, mêmes ressources/topologie, attaque désactivée et sans arrêt par hauteur cible. Aucun échantillon futur attaqué n’est utilisé. Une référence entièrement insuffisante est refusée ; les autres caractéristiques absentes restent ignorées et comptées comme dans M4. La calibration unique ne représente pas l’incertitude de calibration et les seuils sont des paramètres d’ingénierie. Les références JSON externes historiques vérifient fenêtres/audits mais ne certifient pas une correspondance complète des conditions réseau ; l’utilisateur doit conserver le bon profil. Les chemins de fichiers peuvent différer entre exports, sans modifier les trajectoires.

Les données/rapports/configurations M5 restent immuables. Leur moteur 0.4.0 est archivé localement avec DLL et sources, et sa référence est vérifiée par SHA-256. Le protocole prospectif pow_validation_v2 utilise d’autres puissances/seeds et ajoute un contrôle honnête protégé ; il ne remplace pas les benchmarks précédents. Ni le terminal, ni les huit sanity runs ne constituent une preuve statistique générale d’efficacité.


## 0.5.1 — Paramètres simples, affichage et fin d’observation

Le terminal simple demande une puissance adverse q dans [0,100] pour 51 %, selfish et withholding. Le mineur adverse reçoit q unités, les autres mineurs reçoivent ensemble 100-q, au prorata des poids initiaux déterministes. Pas de mineur virtuel ni d’addition de q aux ressources déjà présentes. Les puissances individuelles du mode personnalisé restent celles saisies. La pool simple inclut l’infiltré et le nombre choisi de mineurs honnêtes ; les autres mineurs peuvent être extérieurs à la pool. Sybil choisit identités et réseau ; celles-ci ne créent aucune puissance. Le tirage des poids utilise le même flux indépendant et n’est jamais conditionné à un résultat.

Le paramètre optionnel post_target_ticks (0 par défaut pour les anciens JSON, maximum 10^9) ouvre une fenêtre de production normale à partir de la première hauteur cible locale. Le terminal propose 20*mean_block_ticks comme valeur d’ingénierie, pas comme durée statistiquement suffisante ou réglage optimisé. Le minage reste probabiliste pendant cette période, les plafonds s’appliquent et la fermeture est un événement à timestamp fixé avant de connaître la suite. La chaîne active peut ensuite encore changer pendant le drainage. Les traces et l’export précisent le premier tick de cible et la fin de production. target_height=0 désactive cette condition et conserve le budget global de découvertes.

Après fermeture, les timers mine/share/audit/close_production encore planifiés sont retirés sans avancer le temps ni exécuter les hooks adverses. C’est une correction de l’ancien avancement d’horloge artificiel par des opérations inactives, pas une suppression de données réseau. Les véritables livraisons et remises différées sont toujours traitées. L’audit final ne produit aucune action ; ses alertes sont conservées avec phase=final_observation et operational=false. Un audit sur réception réelle pendant le drainage porte phase=drain. Les scores M4 restent définis sur les familles par run et incluent les observations finales ; ils ne prouvent pas une intervention précoce ni la causalité de chaque anomalie.

L’export distingue outcome, outcome_reason et execution_limit_reached. Une réussite d’objectif déjà observée n’est pas effacée par un plafond atteint ; une non-acceptation, une branche privée non résolue ou une censure restent explicitement non concluantes lorsque l’objectif n’a pas été observé. Les blocs actifs, messages et alertes ne sont pas filtrés en fonction du résultat désiré.

Les traces payment_reversed/payment_restored constatent la présence du paiement accepté dans la vue locale marchande ; elles ne modifient ni l’acceptation mémorisée, ni le consensus, ni la stratégie de l’attaque. p2p_send ajoute la visibilité des envois/retransmissions sans tirage réseau supplémentaire. Le filtrage d’affichage est exclusivement dans src/terminal/display : réceptions cachées en mode synthétique, regroupement des premiers forks/alertes/actions, totaux complets et export conservés. Un test compare byte à byte les JSON issus des affichages synthétique et détaillé.

L’anneau utilisateur de 30 nœuds dépasse souvent le seuil fixe de 40 ticks via des relais honnêtes ; aucune révision opportuniste des seuils n’a été effectuée. Le rapport analysis/TERMINAL_REVIEW_051.md distingue anomalies observables, alertes de classement et pertinence pour la défense, phases d’exécution et limites des 14 essais ciblés. Le nouveau protocole prospectif à budget fixe et l’ancienne référence M5 ne sont pas remplacés par ces essais.


## 0.6.0 — Raffinement M4

Hypothèses, critères, coûts et tests : SECURITY_REFINEMENT_PLAN.md ; mesures réelles : analysis/M4_REFINEMENT_REPORT.md. Les options restent désactivées par défaut et les stratégies M3 sont préservées.

La suspension marchande peut être plafonnée par max_payment_hold_ticks depuis le premier signal d’un épisode de risque ; 0 préserve le renouvellement illimité. L’épisode reste ouvert pendant le renforcement des confirmations. Ce plafond ne borne pas l’attente totale si la chaîne n’atteint jamais le seuil requis. hold_ticks=0 permet les confirmations seules mais désactive aussi les autres suspensions temporisées qui utiliseraient ce champ partagé. La politique ne connaît ni la branche privée ni le futur.

Le journal public merchant_decisions est prélevé avant M3 : présence du paiement dans la chaîne locale, confirmations, seuil effectif et échéances. L’export donne l’acceptation réelle et son index/séquence de trace, les actions strictement antérieures en ticks et causalement antérieures dans la trace ; ces deux compteurs sont null si aucun paiement n’est accepté. Les actions pendant la non-acceptation restent comptées. first_base_ready_at et opportunity_delay_ticks sont des occasions sur la trajectoire réalisée, pas une estimation magique du contre-factuel sans défense.

Les références schéma2 lient les moments honnêtes à un profil initial SHA-256 : topologie/latences/jitter, nœuds/ressources nominales/groupes, cadence/difficulté, fenêtres/audits/min_shares et mécanismes publics pool/paiement. Les ressources sont celles connues par l’expérimentateur ; ce contrôle hors ligne n’est pas une estimation d’un hashrate adverse secret. Les moments par strate publique nœud/composant/métrique sont optionnels. Les labels et branches privés ne participent jamais aux scores. Références liées incompatibles rejetées ; anciennes références non liées autorisées sauf mode strict/stratifié. La déclaration honnête de source n’est pas une signature de confiance contre un opérateur qui falsifierait le fichier.

La variance, EWMA et clipping M4 restent inchangés. Une strate non calibrée est ignorée et comptée ; ne pas traiter ses silences comme des TN fiables. La comparaison à couverture commune et les scores M4 sur familles doivent être distingués des nombres d’alertes locales. Les fenêtres corrélées et une seule seed honnête ne justifient aucune garantie de faux positifs à trois sigma. La limite de référence1 MiB reste applicable ; des profils très grands peuvent être refusés. Une calibration multi-seeds exige encore un merge testé des moments avant de lancer la campagne proposée.

Les reports de probation enregistrent le vrai émetteur et producteur et sont étiquetés seulement après exécution. Leurs catégories se recouvrent. Les shares observées, créditées, non créditées et les unités de récompense sont exportées par rôle d’évaluation ; la perte de pool nécessite un contrôle honnête apparié et ne vaut pas automatiquement le nombre de solutions retenues. Les coûts logiques de stockage des décisions et reports sont comptés ; ce ne sont pas des mesures RSS/CPU.

L’accès aux nouveaux pairs peut être restreint à un annuaire public explicite et retardé. La sélection ne consulte aucun label d’honnêteté. Le lien apparaît seulement à l’événement de connexion ; les demandes en attente sont coalescées. La récupération mesure la première adoption valide après une connexion et conserve null si elle manque. Cela ne prouve ni une convergence complète ni une provenance exclusive au rattrapage. Des pairs initiaux restent admissibles sous quotas même s’ils ne sont pas dans la liste de nouveaux candidats. Les cas sans pair indépendant accessible doivent rester des échecs de mitigation publiés.


## Renforcement M4 `hardened_m4_v1` (optionnel, novembre 2026)

`security.hardened_security=true` active la nouvelle politique ; faux par défaut pour préserver les anciens JSON et le terminal existant. Exemples distincts : `configs/final_defenses/`. Aucune stratégie M3, aucun générateur aléatoire ni règle de travail cumulé n'est changé. Le profil de référence adaptative change de version de caractéristiques : une référence liée à l'ancien extracteur ne peut pas être réutilisée silencieusement.

Détection : un dépassement isolé est `informative`, `persistent` après `persistent_samples` observations consécutives dépassant le seuil (défaut 2), `critical` pour une réorganisation publique de profondeur au moins 2. Une observation normale réinitialise la persistance. Les observations de fenêtres recouvrantes ne sont pas indépendantes ; la persistance n'est pas une probabilité de fraude. L'EWMA et ses garde-fous restent inchangés. Le cooldown des alertes reste applicable. La politique renforcée n'agit pas sur une alerte seulement informative. Les décisions préventives de confirmations et de diversité ne nécessitent pas d'alerte.

Marchand : dès le début, seuil `max(confirmations M3, preventive_confirmations)` (défaut 6) lorsque la défense correspondante est activée. Le propriétaire choisit ce plancher selon la valeur exposée et sa tolérance à l'attente ; 6 est un paramètre pédagogique, pas un seuil statistiquement calibré ni une garantie Bitcoin. Les alertes publiques pertinentes peuvent ajouter des confirmations. Pendant un épisode actif, une alerte répétée ne renouvelle aucun délai ; une profondeur supérieure peut augmenter le seuil sans prolonger l'épisode. Suspension bornée par `max_payment_hold_ticks` si fourni, sinon par `confirmation_ticks` ; durée des confirmations renforcées indépendante. Après expiration, retour au plancher préventif. Journal des décisions et nombre réel de `payment_reversed` exportés ; non-acceptation en fin d'observation reste censurée, jamais une victoire de défense. La branche privée ne fournit aucune caractéristique. Dans un réseau public silencieux mais compatible avec une découverte PoW honnête lente, il peut n'exister aucun signal fiable avant l'acceptation. Une majorité peut encore réussir après toute attente finie.

Publication : les délais des premières réceptions sont regroupés par le producteur annoncé dans la transaction publique `miner:<id>`, jamais par le rôle d'attaquant ni par le mineur conservé dans les métadonnées privées. Au moins deux blocs distincts dans la fenêtre et une alerte persistante sont requis. La probation porte sur le contenu de ce producteur, quel que soit le relais. Elle ne renouvelle pas une période active ; après expiration, une nouvelle période exige de nouveaux blocs observés. Un message retardé est traité après le délai borné, sans probation récursive. C'est une défense expérimentale : elle change le moment de validation locale, conserve le fork choice au travail cumulé, et peut augmenter les forks/retards. L'identité et l'horodatage du producteur sont fidèles dans M3 ; aucune preuve cryptographique anti-usurpation n'est ajoutée. Les seuils de retard restent ceux de la configuration/référence : ils ne prouvent pas seuls une attaque et n'autorisent aucune confiscation ni exclusion permanente.

Pool : N tickets de shares réellement soumis, B blocs complets réellement soumis, moyenne nominale mu=N/16 et déficit Z=max(0,(mu-B)/sqrt(max(1,mu))). En plus du seuil existant de Z et de la persistance, exiger mu>=8 et la borne de queue basse `exp(-mu+B*(1+log(mu/B))) <= pool_tail_probability` (défaut .01), avec B=0 donnant exp(-mu). La borne provient du modèle Bernoulli nominal indépendant de probabilité 1/16 ; les temps exponentiels arrondis du simulateur et les fenêtres recouvrantes empêchent de l'interpréter comme un taux empirique garanti de faux positifs ou un test séquentiel calibré. Aucun nouveau taux de calibration n'est inventé.

Les nouveaux crédits suspects sont séquestrés, pas détruits. Un déficit toujours suffisant avec de nouvelles shares peut maintenir le séquestre temporaire ; les mêmes observations sans nouvelles shares ne prolongent pas l'échéance. Une fenêtre suffisamment remplie et rassurante restitue les crédits, ainsi que l'expiration sans renouvellement. La restitution ajoute réellement les shares aux crédits servant au calcul M3 des payouts. Exporter encours, cumul retenu, restitutions et coût crédit*ticks par membre/rôle (rôle en évaluation après le run seulement). `shares_uncredited` reste le cumul historique des événements de non-crédit, pas l'encours actuel. Le payout M3 est un calcul proportionnel en fin de run, pas un système monétaire à paiements irréversibles : un payout distribuable réduit par séquestre n'est ni une confiscation définitive ni une réparation des blocs perdus par la pool. Les restitutions restent une dette potentielle et doivent être présentées.

Réseau : diversification au démarrage pour le client publiquement désigné par `target` (identique dans le contrôle honnête), sur l'annuaire public, quotas existants de groupes, dernier emplacement réservé à un nouveau groupe accessible si la liste est saturée. Les groupes sont une hypothèse d'admission, pas un oracle d'honnêteté. La redondance active tient lieu de pair de secours ; pas de pair inventé lorsque l'annuaire restreint ne contient que des identités contrôlées. La défense ne reconfigure pas les voisins ni les identités adverses : elle protège un client désigné, pas automatiquement tout le réseau. La topologie et l'annuaire de ce modèle sont statiques : une tentative pour ce client suffit et évite les reconnexions répétitives sans nouveau candidat. Les liens ajoutés utilisent la synchronisation et le délai d'un tick existants ; le délai de bootstrap configuré reste applicable. Les liens sont réciproques : les quotas locaux de sélection ne sont pas une garantie de degré global après toutes les modifications des voisins. Les connexions supplémentaires et messages de synchronisation sont comptés. `initial_eclipse_ticks` mesure après le run la première période de connexions exclusivement adverses, de t=0 jusqu'au premier lien indépendant observé (entrant ou sortant), censurée sinon ; ce n'est pas une mesure de tous les épisodes futurs. `initial_no_block_ticks` mesure séparément l'attente du premier bloc adopté. Une connexion ne prouve pas la convergence.

Évaluation ciblée : protocole prospectif `docs/FINAL_DEFENSE_PROTOCOL.md`, seeds 42 et 43 fixées avant les runs, quatre profils existants copiés dans de nouveaux fichiers sans changer leur stratégie. Horizontes identiques par paire, sans arrêt par hauteur ni nombre de blocs ; contrôle du nombre identique de découvertes. La référence marchande M4 copiée conserve son annonce oracle historique (`public_notice=false`) ; cela concerne la stratégie d'attaque, pas les entrées du défenseur. Ces exemples ne constituent pas une nouvelle estimation multi-seeds, ni une mise à jour M5. Les configurations historiques et les exports M5 sont préservés.
