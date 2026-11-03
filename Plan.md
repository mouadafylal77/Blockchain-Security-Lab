Blockchain-Security-Lab/
│
├── AGENTS.md                  
├── PLAN.md                    # Plan d'architecture validé
├── README.md                  # Installation, build, lancement
├── CMakeLists.txt             # Build C++ principal
├── Makefile                   # Raccourcis : build, test, run-all, figures
├── Dockerfile                 # Environnement reproductible
├── requirements.txt           # pandas, numpy, scikit-learn, matplotlib, scipy
├── .gitignore
│
├── src/
│   ├── main.cpp               # Point d'entrée : ./sim --config xxx.json
│   │
│   ├── core/                  # ===== Atelier 1 (réutilisé) =====
│   │   ├── sha256.hpp/.cpp        # Hash (OpenSSL ou picosha2)
│   │   ├── transaction.hpp/.cpp   # Transactions (id, from, to, montant)
│   │   ├── block.hpp/.cpp         # Bloc, hash, chaînage
│   │   ├── blockchain.hpp/.cpp    # Chaîne, arbre de blocs, forks, orphelins
│   │   ├── consensus.hpp          # Interface ConsensusRule
│   │   ├── pow.hpp/.cpp           # Proof of Work
│   │   ├── pos.hpp/.cpp           # Proof of Stake
│   │   └── fork_choice.hpp/.cpp   # Longest chain, règle Eyal-Sirer, checkpoints
│   │
│   ├── sim/                   # ===== Moteur de simulation =====
│   │   ├── event.hpp              # Types d'événements (BlockFound, BlockReceived...)
│   │   ├── event_queue.hpp/.cpp   # File de priorité triée par temps simulé
│   │   ├── simulator.hpp/.cpp     # Boucle principale + horloge simulée
│   │   ├── rng.hpp/.cpp           # Générateur aléatoire avec seed fixe
│   │   ├── network.hpp/.cpp       # Topologie P2P, latence, perte de paquets
│   │   ├── node.hpp/.cpp          # Nœud : chaîne locale, pairs, mempool
│   │   ├── miner.hpp/.cpp         # Mineur : taux de hash, temps de minage
│   │   ├── honest_miner.hpp/.cpp  # Comportement honnête
│   │   ├── mining_pool.hpp/.cpp   # Pool : shares, répartition des gains
│   │   └── config.hpp/.cpp        # Lecture JSON (nœuds, hash, seed, attaque)
│   │
│   ├── attacks/               # ===== Les 4 attaques =====
│   │   ├── attack.hpp             # Interface Attack
│   │   ├── attack_factory.cpp     # Crée l'attaque selon la config
│   │   ├── attack_51.hpp/.cpp         # Chaîne secrète + double spending + reorg
│   │   ├── selfish_mining.hpp/.cpp    # Stratégie de publication Eyal-Sirer
│   │   ├── block_withholding.hpp/.cpp # Shares envoyées, blocs cachés
│   │   └── sybil.hpp/.cpp             # Fausses identités, isolement (eclipse)
│   │
│   ├── metrics/               # ===== Collecte des métriques =====
│   │   ├── metrics_collector.hpp/.cpp # Orphelins, délais, récompenses, forks
│   │   ├── economic_loss.hpp/.cpp     # Coins volés, récompenses perdues
│   │   ├── overhead.hpp/.cpp          # CPU, mémoire, latence des défenses
│   │   └── csv_exporter.hpp/.cpp      # Export vers results/raw/
│   │
│   ├── detection/             # ===== Contribution : détection adaptative =====
│   │   ├── detector.hpp               # Interface Detector
│   │   ├── feature_extractor.hpp/.cpp # Métriques en fenêtre glissante
│   │   ├── fixed_threshold.hpp/.cpp   # Baseline : seuil fixe
│   │   ├── adaptive_threshold.hpp/.cpp# Moyenne mobile / EWMA / z-score
│   │   ├── ml_bridge.hpp/.cpp         # Charge les prédictions du modèle Python
│   │   └── alert.hpp                  # Alerte (type, nœud, temps, score)
│   │
│   └── mitigation/            # ===== Contre-mesures =====
│       ├── mitigation.hpp             # Interface Mitigation
│       ├── confirmation_policy.cpp    # Confirmations dynamiques (anti-51 %)
│       ├── reorg_limit.cpp            # Limite de profondeur + checkpoints
│       ├── late_block_penalty.cpp     # Anti-selfish mining
│       ├── pool_audit.cpp             # Audit des shares/blocs (anti-withholding)
│       └── sybil_defense.cpp          # Limite de connexions, coût d'entrée, réputation
│
├── configs/                   # ===== Scénarios reproductibles =====
│   ├── baseline.json
│   ├── attack_51.json
│   ├── selfish_mining.json
│   ├── block_withholding.json
│   ├── sybil.json
│   ├── protected_51.json
│   ├── protected_selfish.json
│   └── campaigns/
│       ├── sweep_hashrate.json        # 10 %, 25 %, 33 %, 40 %, 51 %
│       ├── sweep_nodes.json           # 10, 50, 100 nœuds
│       └── sweep_network.json         # latence, perte de paquets
│
├── analysis/                  # ===== Python : analyse =====
│   ├── run_campaign.py            # Lance N runs avec seeds différentes
│   ├── aggregate.py               # Moyennes + intervalles de confiance
│   ├── detection_eval.py          # Précision, rappel, F1, temps de détection
│   ├── ml/
│   │   ├── build_dataset.py       # Métriques → dataset étiqueté
│   │   ├── train_model.py         # Random Forest / Isolation Forest
│   │   ├── evaluate_model.py      # Train/test, ROC, matrice de confusion
│   │   └── model.pkl
│   ├── plots.py                   # Graphiques matplotlib
│   └── run_all.py                 # Pipeline complet
│
├── results/
│   ├── raw/                   # CSV bruts par run (ignorés par git)
│   ├── aggregated/            # CSV agrégés par scénario
│   ├── tables/                # Tableaux comparatifs
│   └── figures/               # PNG/PDF pour le rapport
│
├── dashboard/                 # (optionnel) Streamlit
│   └── app.py
│
├── tests/                     # ===== Tests unitaires =====
│   ├── test_event_queue.cpp
│   ├── test_blockchain_fork.cpp
│   ├── test_baseline_blocktime.cpp
│   ├── test_attack_51.cpp
│   ├── test_selfish_mining.cpp
│   ├── test_block_withholding.cpp
│   ├── test_sybil.cpp
│   ├── test_detectors.cpp
│   ├── test_mitigations.cpp
│   └── test_reproducibility.cpp   # Même seed = mêmes résultats
│
├── scripts/
│   ├── build.sh
│   ├── run_all_experiments.sh
│   └── make_report_assets.sh
│
└── report/
    ├── main.tex               # Rapport technique (LaTeX)
    ├── references.bib
    └── sections/
        ├── introduction.tex
        ├── etat_art.tex
        ├── conception.tex
        ├── contribution.tex
        ├── protocole.tex
        ├── resultats.tex
        └── limites.tex
