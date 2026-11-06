# Terminal simple

Choisir --interactive, puis Mode simple. Six paramètres seulement : nœuds, mineurs, hauteur cible, attaque, puissance adverse pour 51 %/selfish/pool, sécurité aucune/fixe/adaptative. Aucun pourcentage de calcul pour Sybil. Paramètres techniques existants conservés automatiquement ; le mode personnalisé garde leurs réglages et les détails. Le récapitulatif court permet de lancer ou corriger une question. L’exécution simple affiche seulement « Simulation en cours... », y compris pendant une calibration préalable.

Le menu reste ouvert après un résultat ou une comparaison : nouvelle simulation, rejeu, comparaison ou quitter. Comparer exécute réellement les deux essais depuis la même configuration et la même seed, seules les défenses changent. En adaptatif, une seule référence honnête est partagée dans la paire. Une référence honnête économique supplémentaire est réellement exécutée pour calculer la perte de la pool. Les dossiers neufs sans_defense/avec_defense/reference_honnete préservent les JSON complets.

Critères d’affichage (pas de nouvelles règles du moteur) :

| Attaque | Défense efficace OUI dans une comparaison concluante |
|---|---|
| 51 % | Paiement accepté dans les deux essais, renversé sans défense et conservé avec défense. |
| Selfish | Part adverse initialement supérieure à son hash power, puis réduite avec défense. Il s’agit d’atténuation ; l’attaque peut rester réussie. |
| Pool | Part de rémunération adverse dans les récompenses de la pool réduite, avec rétention observée et récompense positive dans les deux essais. Les blocs retenus ne sont pas restitués. |
| Sybil | Victime isolée sans défense, puis adoption observée après reconnexion avec défense. |

NON signifie critère non satisfait dans une comparaison évaluable. NON ÉVALUABLE couvre notamment l’absence de comparaison, les tentatives non concluantes, les censures de temps/événements, un paiement non accepté ou l’absence d’un avantage initial à réduire. Un arrêt prévu au budget de découvertes garde les objectifs finis M3 ; il ne prouve aucune sécurité générale. Une simulation honnête sans attaque est signalée sans inventer un succès.

L’issue OUI/NON/NON CONCLUANTE vient directement de l’issue existante du simulateur. Trois indicateurs maximum par attaque : paiement accepté/annulé/confirmations ; part adverse/avantage en points ; blocs retenus/payout/perte appariée de pool ; isolement/reconnexions/adoption. Une perte de pool négative représente un gain relatif sur cet essai. Les durées réalisées peuvent différer sous une même règle d’arrêt ; aucune efficacité statistique générale n’est inférée.

Moteur, stratégies, contre-mesures, références historiques, seuils et benchmarks inchangés. L’option aucune conserve l’observation dans les exports, sans activer de mitigation. Les anciens JSON et la CLI --config/--output restent disponibles. Le script check-terminal.ps1 adapte uniquement ses saisies au nouveau menu ; aucune campagne scientifique n’est lancée.
