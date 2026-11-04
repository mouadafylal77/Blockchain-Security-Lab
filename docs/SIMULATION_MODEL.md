# Modèle du noyau M1

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
  attendu serait proportionnel à `16^d` ; aucune règle de choix entre branches
  n'est encore implémentée. La longueur ou la majorité de nœuds ne sélectionne
  donc aucune chaîne. Le choix par travail cumulé et ses égalités sont prévus en M2.
- Le minage essaie successivement les nonces depuis zéro, avec budget explicite
  d'un million d'essais par défaut. L'épuisement produit une erreur ; le budget
  ne garantit pas de trouver un bloc, notamment aux fortes difficultés. Le
  minage effectif M1 sera distinct du processus stochastique simulé de M2.
- Un ajout vérifie la PoW, les engagements, l'indice, le parent et le temps avant
  de modifier la chaîne. Aucun mécanisme de consensus distribué n'est présent.

La reproductibilité M1 est une égalité des hashes/nonces pour des entrées
identiques ; il n'y a pas de RNG. Aucune métrique de stale/orphan, récompense,
attaque ou détection n'est encore mesurée. Le PoS de l'Atelier 1 reste uniquement
dans `exo4.cpp` ; le périmètre académique PoS nécessite une clarification avant
les scénarios d'attaques. Aucune extrapolation des résultats PoW au PoS n'est faite.
