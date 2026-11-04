# Modèle de Blockchain Security Lab

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
