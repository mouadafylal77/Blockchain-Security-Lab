# M4 0.6.0 — Raffinement scientifique et essais ciblés

Les hypothèses, coûts et critères ont été écrits avant les modifications dans `docs/SECURITY_REFINEMENT_PLAN.md`. Trois lots ciblés, aucune campagne M5, aucun changement de stratégie M3. Les résultats sont des contrôles de développement sur des seeds déclarées, pas des estimations générales.

## A — Politique et causalité marchandes

La séparation hold_ticks/confirmation_ticks existait déjà et a été conservée. Ajout optionnel de max_payment_hold_ticks : 0 garde le renouvellement illimité historique ; une valeur positive borne la suspension depuis le début de l’épisode de risque. Les confirmations renforcées peuvent continuer au-delà de ce plafond. Un nouvel épisode commence après expiration du renforcement. hold_ticks=0 permet une politique de confirmations seules ; ce paramètre étant partagé, ne pas activer en parallèle d’autres suspensions temporisées si l’on veut conserver leur durée habituelle.

Le journal public marchand, calculé avant l’évaluation M3, contient présence du paiement, tête locale, confirmations de base/effectives, suspension, échéances, éligibilité, tick et séquence. Il n’utilise ni chaîne privée ni connaissance future. L’export distingue strictement avant par tick et causalement avant par ordre des traces, y compris au même tick ; sans acceptation, les compteurs « avant acceptation » sont null et les actions pendant la non-acceptation restent comptées. first_base_ready_at est une occasion sur la trajectoire réalisée, pas un contre-factuel sans défense.

| Cas contrôlé | Observation réelle |
|---|---|
| Signal public précoce, seed42, poids10/45/45, liens40, base6 | Première action marchande190 ; première occasion de base206 ; acceptation377 sous seuil renforcé ; 171 ticks d’attente supplémentaire sur cette trajectoire. Objectif adverse non observé, branche privée restante : non concluant, pas protection prouvée. |
| Alerte tardive, seed42, majorité85 %, notice publique | Paiement accepté74 ; première action marchande85 ; zéro action causale avant acceptation ; double dépense réussie. |
| Honnête sans défense | Acceptation206. |
| Honnête, suspensions renouvelées sans plafond | Aucune acceptation ; valeur de délai null/censurée, pas succès défensif. |
| Même stress honnête, plafond50 | Acceptation512 ; 306 ticks de plus que le contrôle honnête ; mêmes2748 alertes. |

Le dernier stress utilise volontairement silence_ticks=1 et cooldown=1 pour exercer les refus répétés ; ce réglage n’est ni un seuil recommandé ni un benchmark représentatif. La politique plafonnée améliore l’acceptation dans ce cas de couverture, mais peut aussi permettre des paiements risqués : aucune réduction générale du taux de double dépense n’est démontrée. Les signaux publics disponibles avant acceptation restent forks et silences ; ils sont aussi possibles honnêtement. Une réorganisation observée après une publication secrète ne constitue jamais une preuve publique antérieure à cette publication.

## B — Référence honnête liée au réseau

Options stratify_reference et strict_reference désactivées par défaut pour compatibilité. En mode stratifié, les moments honnêtes sont indexés par nœud/composant/métrique, pas par label d’attaque. L’EWMA, le score avant mise à jour, les bornes de contamination et les seuils fixes ne sont pas changés. Une strate absente ne reçoit pas de valeur inventée : la mesure est ignorée et comptée.

Les nouveaux exports de calibration (schéma2) portent un SHA-256 du profil : nœuds/poids nominaux/groupes, liens/latences/jitter, difficulté/cadence, fenêtre/audit, audit événementiel, min_shares, pool et mécanisme public de paiement. Seed, activation de l’attaque/défense, chemin et budget de calibration sont exclus ; les références peuvent donc servir aux modes appariés. Les poids nominaux sont connus par l’expérimentateur dans ce laboratoire, pas estimés secrètement par le détecteur. Le fingerprint sert au contrôle hors ligne des conditions, jamais comme caractéristique d’alerte. Il décrit le contexte initial, pas toutes les modifications de réseau ultérieures dues aux défenses.

Toute référence liée portant un profil différent est refusée. Les références historiques sans profil restent lisibles sauf strict_reference=true ou stratification demandée. La déclaration de source honnête est vérifiée (mode calibration, attaque désactivée), mais n’est pas une attestation cryptographique de données externes.

Calibration réelle : seed41001, 30 nœuds/10 mineurs, anneau5+jitter2, 80 découvertes maximum, fenêtre2000, audits100. Validation honnête indépendante : seed42001, mêmes conditions. Deux exécutions sans défense et deux honnêtes protégées.

| Mesure de validation | Fixe | Adaptatif stratifié |
|---|---:|---:|
| Alertes locales totales | 826 | 265 |
| Alertes de retard late_p95 | 544 | 14 |
| Alertes reorg_depth | 258 | 227 |
| Alertes silence_ticks | 24 | 24 |
| Actions sur participants honnêtes, mode protégé | 258 | 227 |
| Acceptation marchande | 119 | 119 |

Limite importante : 227 cellules de référence et 336 évaluations de strates insuffisantes/absentes. Les blocs produits des validations sans défense sont identiques. Sur les seules clés couvertes, le fixe produit822 alertes (540 de retard), contre265 pour le stratifié ; quatre alertes fixes étaient sur des clés non couvertes. La baisse n’est donc pas uniquement due aux abstentions, mais celles-ci doivent rester visibles. Aucune conclusion de taux de faux positifs général à partir d’un seul run : au sens des familles M4, les mêmes trois familles restent présentes honnêtement dans les deux modes. La disponibilité du paiement n’est pas améliorée dans ce profil, accepté avant les premières actions marchandes. Les 227 réorganisations honnêtes alertées restent un problème.

Un test synthétique honnête à deux pairs (retards habituels10 et100) montre qu’une strate normale lente ne masque pas une anomalie60 chez le pair normalement rapide ; le seuil fixe40 est conservé. Les tests rejettent un mauvais lien/cadence/profil de stratification, vérifient l’indépendance de la seed du fingerprint et l’absence de données futures. La référence unique ne mesure pas l’incertitude d’une calibration multi-seeds ; les fenêtres corrélées n’autorisent aucune promesse gaussienne « trois sigma ». La limite de fichier1 MiB est conservée, pouvant limiter de très grands profils stratifiés.

## C — Coûts opérationnels et bootstrap moins favorable

Les rôles honnêtes/adverses sont appliqués seulement dans l’export après simulation. Les détecteurs et politiques ne reçoivent jamais ces étiquettes.

| Défense / cas seed42 | Bénéfice ou résultat | Coût ou limite observée |
|---|---|---|
| Probation selfish, profil M4 inchangé | Part adverse55,22→44,83 %, toujours >40 % | 141 messages différés, dont94 venant d’émetteurs honnêtes et46 de producteurs honnêtes ; 4600 ticks ajoutés cumulés à ces contenus honnêtes. |
| Audit pool, profil M4 inchangé | Payout adverse17,8966→6,9579 ; payout honnête30,1034→41,0421 unités | 371 shares adverses non créditées, zéro honnête sur cet essai ; récompense pool48 inchangée. |
| Pool honnête appariée | Récompense99 unités | Perte appariée51 unités sous attaque, alors que52 solutions ont été retenues : ces quantités ne sont pas interchangeables. |
| Faux audit de pool synthétique | Cas entièrement honnête, seuil0,01 et min_shares1 explicitement réservés au test | 767 shares honnêtes non créditées ; le mécanisme ne prouve donc pas la fraude. |
| Sybil, candidat public indépendant accessible | Demande100, connexion100, adoption101 | Une reconnexion ; dépend de l’annuaire et des groupes supposés fiables. |
| Même Sybil avec délai250 | Connexion350, adoption351 | 251 ticks depuis la demande ; le délai modifie réellement l’accessibilité du lien. |
| Sybil, seuls relais contrôlés accessibles | Cinq demandes, zéro connexion nouvelle, aucune adoption | Isolement réussi ; récupération null/non observée, pas délai zéro ni réparation omnisciente. |

Les catégories émetteur/producteur se recouvrent ; ne pas additionner94+46 comme deux populations disjointes. Les ticks cumulés de messages ne sont ni un chemin critique ni du CPU. La récompense est une unité pédagogique de pool, pas une monnaie réelle. Le coût CPU/RSS propre au collecteur n’est pas profilé ; tailles logiques de journaux et références sont exportées. La nouvelle instrumentation ajoute traitement et mémoire, à considérer dans la future campagne.

restrict_bootstrap, bootstrap_peers et bootstrap_delay_ticks sont des paramètres publics d’environnement ; défauts : annuaire non restreint et zéro délai supplémentaire. Les candidats existants sont soumis aux mêmes quotas ; aucune sélection d’après leur honnêteté réelle. Un raccordement différé n’active aucun lien avant son événement, et plusieurs demandes en attente sont coalescées. La première adoption après connexion ne signifie ni convergence complète ni attribution exclusive au rattrapage. Les pairs initialement connus peuvent rester présents malgré les restrictions d’accès à de nouveaux candidats.

## Vérification et artefacts

- Avant : 60/60 tests, 21,92 s. Lot A :61/61,23,26 s. Lot B :62/62,23,38 s. Lot C :65/65,37,07 s. Dernière suite après précision des compteurs causaux : **65/65,27,45 s**, sans erreur ni warning de compilation relevé. Les durées de suites ne sont pas des mesures du coût des défenses.
- Commande : `./scripts/test-ucrt64.ps1`, CMake/Ninja/GCC UCRT64, deux tâches de compilation maximum.
- Résultats détaillés de développement : `build/ucrt64/security-refinement/`, notamment merchant_early, merchant_late, honest_*, network30/*, probation_*, pool_*, sybil_*.
- Cinq nouveaux exemples JSON réellement exécutés via la CLI, tous code0 : configs/security_refinement/{merchant_capped,network_calibration,network_validation,sybil_restricted,sybil_delayed}.json ; sorties sous results/raw/security_refinement/. Ils sont des exemples, pas des configurations optimisées ni des résultats de campagne.
- Une empreinte avant/après couvre exo4.cpp, tous les fichiers M1 et M3 et toutes les configurations préexistantes : aucune divergence. Les champs nouveaux ont leurs valeurs par défaut pour les anciens JSON. M5 est vérifié séparément par scripts/preserve-m5.ps1. Aucun commit/push.

La proposition `analysis/M4_INDEPENDENT_BENCHMARK_PROPOSAL.md` n’a pas été exécutée. Elle demande validation du propriétaire sur profils, tolérance de sanctions honnêtes, grille de politiques, seeds et budget mesuré avant grande campagne. Elle requiert aussi un merge testé des moments pour une véritable calibration multi-seeds, encore reporté. Les essais actuels ne démontrent ni efficacité générale ni protection garantie contre une majorité PoW.
