# Terminal 0.5.1 — essais ciblés des quatre attaques

Audit et résultats réellement observés, seed 42. Aucun benchmark M5 relancé, aucun seuil statistique modifié. Les résultats ci-dessous ne sont pas des estimations multi-seeds.

## Diagnostic de l’exécution utilisateur (30 nœuds / 10 mineurs / hauteur 15)

Source conservée : `results/raw/interactive/run_2/results.json`, SHA-256 `c94f7766cc92eb17f5a5f95a784aca0b2e95f386d866c908568e278f81c64c71`.

La génération 0.5.0 affectait des poids uniformes indépendants : adversaire 34 / total 498, soit 6,827309 %. La nouvelle interface simple demande explicitement le pourcentage et répartit le complément entre les autres mineurs ; elle ne change pas les anciens JSON. Le mode personnalisé conserve les poids individuels.

L’exécution utilisait le détecteur **fixe**, sans référence de calibration (`initial_reference={}`), fenêtre 2000, audit 10 ticks, cooldown 500, seuil late_p95=40. L’anneau a 30 nœuds, latence 5 et jitter 0..2 par saut. Les délais mesurés sont moyenne 46,1724 et maximum 91 ticks sur 464 arrivées : dépasser 40 ne suffit donc pas à prouver une rétention adverse.

| Alertes originales | Nombre | Lecture démontrée |
|---|---:|---|
| late_p95 / publication_anomaly | 141 | Les deux blocs adverses restent privés ; les contenus arrivés sont honnêtes. Le retard de bout en bout via des relais explique des dépassements sans publication malveillante. |
| reorg_depth / chain_instability | 31 | Forks/réorganisations entre blocs honnêtes ; aucune chaîne privée publiée. Instabilité réelle, mais pas preuve de double dépense. |
| silence_ticks / connectivity_anomaly | 30 | Une par nœud au dernier audit non opérationnel, tick 5312, après fin de production 2349 et dernière livraison 2407. Artefact des minuteries inactives. |

Les 202 alertes concernent les 30 nœuds, maximum 10 par nœud ; 71 séries distinctes (nœud, composant, métrique). Dix-sept séries apparaissent cinq fois, seize quatre fois, sept trois fois, une deux fois et trente une fois. Le cooldown est local à une série, pas global au réseau. Les alertes répétées sont conservées, pas supprimées des exports.

Les 31 actions correspondent à l’instabilité de chaîne, pas à 31 attaques détectées. Le marchand node29 n’a aucune alerte reorg_depth ; son paiement est accepté au tick 334 à 2/2 confirmations. Les signaux d’autres nœuds ne protègent pas automatiquement ce marchand. Les alertes finales au tick 5312 n’appliquent aucune action. Les « faux positifs » de classement M4 ne sont pas nécessairement des observations réseau fausses : leur causalité/pertinence doit être distinguée du label de famille par run.

## Correction d’horloge et observation après cible

Les minuteries de minage, shares et audits devenues inactives sont désormais annulées sans avancer l’horloge. Aucune attente réelle ni modification des tirages de découverte. Les phases d’alerte sont exportées : production, drain et final_observation ; cette dernière n’applique aucune mitigation. Les alertes finales restent comptées, avec leur contexte explicite.

Le rejeu **exact du JSON original** sous 0.5.1 conserve l’ensemble des blocs produits, l’acceptation au tick 334 et les délais de propagation. L’horloge finit à 2407 ; 11 minuteries annulées. Alertes : 178, métriques {'late_p95': 142, 'reorg_depth': 36}, phases {'production': 171, 'final_observation': 7} ; 30 actions. La différence 31→30 actions tient à l’annulation de l’audit périodique déjà planifié après fermeture. Les sept alertes finales ne constituent pas sept actions. Le nombre de reorg_depth reste sensible à la fenêtre et au moment d’observation final : il n’a pas été réduit en changeant un seuil.

`post_target_ticks` permet de continuer la production **normale** pendant une fenêtre bornée après la première hauteur cible, avant drainage. Défaut du nouveau terminal : 20 intervalles moyens (2000 ticks, 4000 pour la pool) ; 0 conserve l’arrêt à la cible. Les limites dures gardent priorité. Aucune publication privée forcée et aucune prolongation jusqu’au succès.

Le prolongement du JSON original de 2000 ticks finit au tick 4433, avec 36 découvertes, hauteur 31, toujours 2 blocs privés et zéro publication : **non concluant**. La fermeture précoce censure une tentative encore ouverte, mais la puissance de 6,83 % et cette trajectoire ne fournissent aucune preuve qu’un délai supplémentaire raisonnable aurait suffi à réussir.

Une session réelle du nouveau terminal (30/10/15, 40 %, défenses fixes, seed42, +2000 ticks) donne 44 découvertes, hauteur 26, 16 blocs privés non publiés : non concluant. 399 alertes, 84 actions ; pas de réduction arbitraire des seuils. La sortie complète du terminal compte 144 lignes (questions et récapitulatif compris), avec 4742 réceptions/répétitions masquées seulement à l’écran.

## Huit comparaisons simples réelles

Configurations identiques par paire sauf protection. Cinq nœuds, deux mineurs, seed42, anneau, latence5+jitter0..2. Pour 51 %, selfish et pool : 40/60 % de puissance, aucune puissance virtuelle supplémentaire. Hauteur30 et fenêtre2000 ; pool : difficulté2, intervalle200, deux membres et fenêtre4000. Sybil : deux identités sans hash power, cible passive node4, capture initiale, hauteur15, fenêtre2000. Détecteur fixe actif en observation sans défense. Les seuils et mitigations M4 ne sont pas réoptimisés.

| Attaque | Sans défense | Avec défense | Alertes sans/avec | Actions sans/avec | Effet mesuré |
|---|---|---|---:|---:|---|
| Double dépense 40 % | réussite M3 | réussite M3 | 45/45 | 0/24 | Paiement accepté419, publication425, renversé431 ; première alerte marchande431, trop tard |
| Selfish Mining | réussite M3 | réussite M3 | 216/231 | 0/73 | Part adverse 60.42→55.32 % ; 247 reports |
| Block Withholding | réussite M3 | réussite M3 | 110/110 | 0/25 | 47 solutions retenues dans les deux cas ; payout adverse 16,3729→6,7622 unités ; récompense pool44 inchangée |
| Sybil | réussite M3 | échec M3 | 106/5 | 0/2 | Cible au genesis sans défense ; 2 reconnexions, puis reprise |

- **51 %** : trois blocs privés sont retenus puis publiés. Une réorganisation retire le paiement réellement accepté ; l’audit événementiel réagit au tick de renversement mais pas avant le paiement passé. Alertes : 24 reorg_depth, 16 late_p95, 5 stale_ratio par mode. Le contrôle supplémentaire à 85 % réussit également dans les deux modes (acceptation378, publication384, renversement390, alerte marchande394), sans promesse de protection majoritaire. À l’inverse, les tests incluent puissance nulle et non-acceptation non concluante. Une alerte globale n’est pas une intervention marchande précoce.
- **Selfish** : 35 blocs retenus/publiés sans défense ; 37 retenus, 35 publiés et deux encore privés avec défense, avance maximale7. Les deux résultats restent des réussites au sens M3 (part active >40 % avec publication), malgré la baisse de part sur cette seed. Alertes sans : {'reorg_depth': 70, 'fork_rate': 5, 'stale_ratio': 70, 'late_p95': 56, 'silence_ticks': 15}, avec : {'reorg_depth': 75, 'fork_rate': 5, 'stale_ratio': 70, 'late_p95': 74, 'silence_ticks': 7}. Les 247 reports incluent 118 contenus de producteurs honnêtes. La probation est expérimentale et peut nuire aux relais/payloads honnêtes. Les alertes finales non opérationnelles restent visibles.
- **Withholding** : 483 shares observées chez l’infiltré et 815 chez le mineur honnête. Première alerte de déficit au tick 5130. Les 25 alertes share_deficit activent la suspension de crédit ; 335 shares adverses non créditées, zéro share honnête non créditée dans cet essai. Les 85 autres alertes sont des silences ; avec un seul mineur publiant et intervalle public attendu supérieur à l’intervalle global, elles ne prouvent pas une fraude. Payout honnête 27,6271→37,2378 unités. Les 47 solutions retenues et la récompense pool44 ne sont pas restaurées. Le statut M3 « réussite » signifie rétention effective, pas absence d’effet économique de la défense.
- **Sybil** : filtrage effectif vers la cible. Sans défense, la cible reste au genesis et le plafond de 300 découvertes est atteint ; réussite de l’objectif d’isolement **et** limite d’exécution déclarée. Avec défense, silence détecté, première reconnexion tick 510, première adoption tick 511, hauteur cible atteinte1537 ; deux reconnexions et rattrapage réel. Quatre alertes de retard accompagnent ensuite la reconnexion ; elles ne sont pas une nouvelle preuve d’attaque. L’échec d’isolement dépend ici de l’annuaire public et des groupes administratifs supposés fiables.

Les hauteurs cibles donnent des durées de production dépendantes de la trajectoire. Les paires n’ont donc pas nécessairement le même horizon réalisé : particulièrement Sybil (300 découvertes sans défense contre 29 avec). Ce tableau ne fournit ni intervalle de confiance ni estimation générale d’efficacité. Pour une comparaison statistique, choisir un horizon/budget commun préenregistré, contrôler calibration et répéter les seeds ; ne pas mélanger ces essais avec M5.

## Vérifications personnalisées et adaptatives

Quatre sessions CLI personnalisées protégées : poids individuels40/60, seed42, cible node4, topologie ligne, latence7/jitter3. Confirmations6 pour 51 %, intervalle50 sauf pool200, plafond100, fenêtre post-cible0 ; la pool conserve ses deux membres ; Sybil n’a qu’une identité et capture=false. Résultats : 51 %, selfish et rétention pool observés ; Sybil échoue **sans action de défense**, car une liaison honnête subsiste déjà. Ce contre-exemple ne doit pas être attribué à la mitigation. Les seuils de chaque attaque et tous ces paramètres sont vérifiés dans les tests et les JSON exportés.

Une session simple adaptative réelle distincte (5/2, hauteur8, 40 %, seed42, sans prolongement) exécute d’abord la calibration honnête séparée avec seed dérivée, puis charge ses moments. Résultat observé : success, 23 alertes, 10 actions ; la réaction ne sauve pas un paiement déjà accepté. Ce test d’intégration n’est pas une calibration optimisée ni une comparaison statistique fixe/adaptatif.

## Preuves et commandes

- `./scripts/test-ucrt64.ps1` : 54/54 avant modification (7,75 s), 60/60 après correction (16,34 s), compilations limitées à deux tâches. Les tests couvrent aide, retour/correction au récapitulatif, pourcentages, limites et entrées invalides par attaque, interfaces simple/personnalisée, actions réelles, prolongement borné, horloge sans minuteries inactives et égalité JSON entre affichage synthétique/détaillé.
- `./scripts/check-terminal.ps1` : 14 sessions CLI code0, 8 simples +4 personnalisées +2 contrôles majoritaires. Le script injecte puis corrige au moins une entrée invalide par attaque simple. Entrées, stdout, manifeste SHA-256 et résumé : `results/raw/terminal_review/checks_1/`. Résultats originaux des sessions : `results/raw/interactive/run_3` à `run_16`.
- Session 30 nœuds et session adaptative : `run_17` et `run_18`, logs `build/terminal-30nodes.log`, `build/terminal-adaptive-demo.log`.
- Rejeux exact/prolongé de l’ancien cas : `results/raw/terminal_review/user_replay_051/` et `user_extended_051/`. L’original run_2 n’est pas modifié.
- M1, règles de consensus M2, stratégies M3 et seuils/contre-mesures M4 conservés. Modification d’ordonnancement uniquement pour l’arrêt/annulation des tâches inactives et la fenêtre explicite. Les événements payment_reversed/restored lisent l’état local marchand sans modifier sa décision. L’affichage lit les événements ; les exports restent complets et les détecteurs n’accèdent jamais à cette vue privée.
- Aucun commit/push, aucun backend/ML, aucune campagne M5 relancée. Vérification d’intégrité M5 à la livraison.

## Limites et prochaines améliorations

Les seuils fixes sont peu adaptés à certains grands diamètres et à certaines cadences ; ne pas les réduire après observation pour embellir le bilan. Les multiples observations locales et le cooldown expliquent une grande part du volume. Il reste à évaluer des références honnêtes représentatives, les coûts des fausses sanctions et une unité d’alerte opérationnelle distincte des familles d’évaluation M4. Une branche privée en attente demeure non concluante au budget choisi ; une durée plus longue n’assure jamais un succès. Les modèles selfish/pool/Sybil restent pédagogiques, le partage privé d’une coalition instantané, et les mécanismes commerciaux/signatures abstraits. Aucune protection garantie contre une majorité de hash power.
