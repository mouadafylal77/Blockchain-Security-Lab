# PLAN.md — Blockchain Security Lab (Atelier 2)

> **Status:** working architectural plan based on the modular structure proposed by Claude and the decisions from project discussions. **Not yet validated against the full official `projet.md`.** Where it differs, the professor's requirements prevail.
>
> **Immediate goal:** first compile and test a minimal C++ blockchain engine, then add the discrete-event multi-node simulator and security experiments in increments.

## 1. Purpose and expected deliverables

Build an academic platform to **simulate**, **observe**, **detect** and **evaluate mitigations** for four blockchain attacks:

1. **51% / double-spend / chain reorganization:** adversary privately mines a competing PoW branch and attempts a reorganization and reversal of a previously observed payment.
2. **Selfish mining:** adversary strategically withholds and publishes blocks to improve its relative reward share.
3. **Block withholding:** malicious participant in a mining pool submits shares but withholds eligible full blocks, harming pool rewards.
4. **Sybil:** attacker introduces multiple network identities to influence peer connectivity and information flow; no automatic increase in hash power.

Compare **baseline**, **attacked**, and **protected** runs. Each experiment needs explicit parameters, a fixed RNG seed, well-defined observations, and reproducible exported results. Expected measurements include block propagation delays, stale/fork activity, chain divergence/reorganizations, miner/pool rewards, suspicious node activity, attack success, detection precision/recall/latency, economic impact, and mitigation overhead **where meaningful and measurable**. Definitions must be documented before numerical comparisons.

### Out of scope until needed
A full cryptocurrency wallet, real token value, production P2P networking, real-world proof-of-stake security, distributed deployment, and an ML detection system are not assumed. Real network packet handling is unnecessary for the initial **simulated** P2P network.

## 2. Technology stack

| Layer | Technology | Phase |
|---|---|---|
| Simulation and blockchain | C++17; modular `.hpp/.cpp` | 1–5 |
| Build/test | CMake, CTest, available C++ compiler | 1–5 |
| Hashing | OpenSSL SHA-256 | 1–5 |
| Configurations / outputs | JSON input; CSV/JSON output | 1–5 |
| Experiment automation / statistics | Python, pandas, NumPy, SciPy, Matplotlib | 5–6 |
| Optional ML research | scikit-learn | Only if justified |
| Future web API | Python, FastAPI | 7 |
| Future web UI | HTML, CSS, JavaScript; Retro Web 1998 | 7 |
| Version control | Git / GitHub | All phases |

The CLI and engine must not depend on Python or a web server. A `requirements.txt` file becomes relevant when Python analysis or FastAPI is actually introduced.

## 3. Target repository structure

This is the **target structure**, not an instruction to generate all files immediately. Modules, configurations and tests are added as each milestone is implemented. The source tree follows Claude's proposal, with a later FastAPI + custom web UI instead of Streamlit as the principal interface.

```text
Blockchain-Security-Lab/
├── AGENTS.md
├── PLAN.md
├── projet.md                       # Professor's official assignment
├── exo4.cpp                        # Original Atelier 1 source: unchanged
├── README.md
├── CMakeLists.txt
├── .gitignore
├── src/
│   ├── main.cpp
│   ├── core/
│   │   ├── sha256.hpp / sha256.cpp
│   │   ├── transaction.hpp / transaction.cpp
│   │   ├── block.hpp / block.cpp
│   │   ├── blockchain.hpp / blockchain.cpp
│   │   ├── consensus.hpp
│   │   ├── pow.hpp / pow.cpp
│   │   ├── pos.hpp / pos.cpp            # Simplified, optional/educational
│   │   └── fork_choice.hpp / fork_choice.cpp
│   ├── sim/
│   │   ├── event.hpp
│   │   ├── event_queue.hpp / event_queue.cpp
│   │   ├── simulator.hpp / simulator.cpp
│   │   ├── rng.hpp / rng.cpp
│   │   ├── network.hpp / network.cpp
│   │   ├── node.hpp / node.cpp
│   │   ├── miner.hpp / miner.cpp
│   │   ├── honest_miner.hpp / honest_miner.cpp
│   │   ├── mining_pool.hpp / mining_pool.cpp
│   │   └── config.hpp / config.cpp
│   ├── attacks/
│   │   ├── attack.hpp
│   │   ├── attack_factory.cpp
│   │   ├── attack_51.hpp / attack_51.cpp
│   │   ├── selfish_mining.hpp / selfish_mining.cpp
│   │   ├── block_withholding.hpp / block_withholding.cpp
│   │   └── sybil.hpp / sybil.cpp
│   ├── metrics/
│   │   ├── metrics_collector.hpp / metrics_collector.cpp
│   │   ├── economic_loss.hpp / economic_loss.cpp
│   │   ├── overhead.hpp / overhead.cpp
│   │   └── csv_exporter.hpp / csv_exporter.cpp
│   ├── detection/
│   │   ├── detector.hpp
│   │   ├── feature_extractor.hpp / feature_extractor.cpp
│   │   ├── fixed_threshold.hpp / fixed_threshold.cpp
│   │   ├── adaptive_threshold.hpp / adaptive_threshold.cpp
│   │   └── alert.hpp
│   └── mitigation/
│       ├── mitigation.hpp
│       ├── confirmation_policy.cpp
│       ├── reorg_limit.cpp
│       ├── late_block_penalty.cpp
│       ├── pool_audit.cpp
│       └── sybil_defense.cpp
├── configs/
│   ├── baseline.json
│   ├── attack_51.json
│   ├── selfish_mining.json
│   ├── block_withholding.json
│   ├── sybil.json
│   ├── protected_51.json
│   ├── protected_selfish.json
│   └── campaigns/
├── tests/
│   ├── test_blockchain.cpp
│   ├── test_event_queue.cpp
│   ├── test_network_forks.cpp
│   ├── test_attack_51.cpp
│   ├── test_selfish_mining.cpp
│   ├── test_block_withholding.cpp
│   ├── test_sybil.cpp
│   ├── test_detectors.cpp
│   ├── test_mitigations.cpp
│   └── test_reproducibility.cpp
├── analysis/
│   ├── run_campaign.py
│   ├── aggregate.py
│   ├── detection_eval.py
│   ├── plots.py
│   └── ml/                         # Optional, later
├── results/
│   ├── raw/
│   ├── aggregated/
│   ├── tables/
│   └── figures/
├── scripts/
├── docs/
│   ├── SIMULATION_MODEL.md
│   └── DEVELOPMENT_LOG.md
├── backend/                        # Later: FastAPI
├── frontend/                       # Later: retro HTML/CSS/JS
└── report/                         # Later: scientific report
```

**Notes on consistency:** `PLAN.md` is the architectural reference; a separate `docs/ARCHITECTURE.md` is not necessary unless it provides useful low-level diagrams rather than duplicating this plan. Optional Makefile, Dockerfile, Streamlit dashboard, ML bridge, and LaTeX report tooling should only be added if needed. The repository name remains `Blockchain-Security-Lab`.

## 4. Simulation model and scientific design

### 4.1 Blockchain and consensus
- Blocks include identifiable parent links, timestamps in **simulated time**, content/transactions, difficulty/work metadata and hashes.
- Each node has a **local view** of the block tree (not one universally shared mutable chain).
- Model PoW block discovery with an explicit, seeded stochastic process. Document whether block finds use an exponential waiting-time model and how discovery events are rescheduled after tip changes.
- Define canonical-chain selection by **cumulative work** with deterministic tie-breaking. A greater number of nodes does not imply greater cumulative work.
- Maintain clear distinctions between a stale valid block, an unknown-parent block, and a confirmed invalid block. Define reorganization depth and confirmation counts.
- Preserve Atelier 1 PoS as a separate simplified educational component only if appropriate; do not treat it as the consensus used for PoW attack evaluations.

### 4.2 Network and discrete-event execution
- Central event queue sorted by `(simulated_time, deterministic_sequence_id)`.
- Event types may include block found, block received, transaction broadcast, peer connection, and detector/mitigation actions.
- Nodes have local chain state, peers, pending transactions and mining configuration.
- Network simulation models topology, message latency and optionally loss/drop policies. Keep assumptions explicit.
- RNG is seeded and controlled; avoid reading wall-clock time as a simulation input.
- Keep simulated overhead (e.g., message counts / additional propagation) distinct from real CPU time and actual memory usage.

### 4.3 Security attacks: scenario semantics
- **51%:** define adversarial hash-power fraction, the target transaction, merchant confirmation threshold, the attacker’s private branch, and an objective attack-success condition. Do not equate 51% of nodes with 51% hash power.
- **Selfish mining:** model private lead and publication strategy, competing discoveries, race resolution and reward accounting. Treat Eyal–Sirer as an attack-strategy reference rather than a general consensus fork-choice rule.
- **Block withholding:** distinguish low-difficulty pool shares from fully valid network blocks, and define how withheld blocks affect pool rewards, participant payouts and detectability.
- **Sybil:** define identity budget and manipulated peer connections (and possible eclipse-like isolation if selected), explicitly separating identity count from mining resources.

### 4.4 Detection and mitigation
- First establish a **fixed-threshold baseline**, then implement an **adaptive detector** (e.g., EWMA or rolling statistics) with specified observation windows and alert semantics.
- Use a shared alert format (timestamp, targeted node/pool, detector, score and attack label where appropriate).
- Evaluate precision/recall only with a clearly defined ground-truth labeling scheme and observation unit (events, windows, or runs).
- Mitigations are experiment-specific and must state limitations:
  - 51%: configurable confirmation policy; optional reorganization constraints/checkpoints flagged as consensus-changing experimental choices.
  - Selfish mining: study propagation and proposed penalties only under precisely documented assumptions.
  - Pool withholding: pool-share accounting/auditing with acknowledged limits of detection.
  - Sybil: peer diversity, connection limits and identity/admission costs under a stated threat model.
- Never assume a proposed countermeasure completely prevents an attack.

### 4.5 Reproducibility and evaluation
- Record experiment name, scenario, parameters, seed, software/build version and measurement definitions.
- Run **baseline / attacked / protected** with controlled comparable conditions; repeat across independent seeds.
- Report attack success rate, reward changes, divergence/reorganizations, propagation delays, detection latency and performance costs where valid.
- Report uncertainty (e.g., confidence intervals) where sample size supports it; do not present unexecuted or invented numerical results.
- For economic loss, specify whether the unit means simulated coin value, lost mining rewards or a separate proxy metric.

## 5. Development milestones and acceptance criteria

### M0 — Repository audit and requirements alignment
**Deliverables:** inspect `projet.md`, `exo4.cpp`, `AGENTS.md`, this plan; note missing requirements and any mismatches in `docs/DEVELOPMENT_LOG.md`. Choose a JSON parser and minimal test style only when implementing them.

**Done when:** Codex can state a traceable checklist of compulsory functions and the initial build dependencies. No mass file generation.

### M1 — Minimal C++ blockchain core
**Build:** root CMake project, minimal CLI, SHA-256 wrapper, block and blockchain validation with a simple PoW path. Refactor concepts from `exo4.cpp` into new files, keeping original untouched.

**Tests:** known SHA-256 vector, valid block/chain, invalid previous hash, modified block/transaction, deterministic chain selection/ties as applicable.

**Done when:** project compiles and the automated tests run successfully in the available environment; an example command is documented.

### M2 — Discrete-event simulator and multi-node network
**Build:** deterministic RNG, event queue, time advancement, nodes with separate local block trees, miners with configurable hash power, simulated P2P propagation, fork resolution.

**Tests:** queue ordering, same seed reproducibility, network delivery, concurrent competing blocks, cumulative-work chain choice, node divergence/convergence.

**Done when:** a baseline scenario produces measurable block discovery, propagation and fork events in a reproducible run.

### M3 — Attack scenarios, one at a time
**Suggested order:** 51% / double-spend → selfish mining → block withholding → Sybil.

For each: scenario configuration, attack implementation, success-condition assertions, normal-behavior control case, automated tests and raw metrics export. Do not begin all four attacks concurrently.

**Done when:** each attack demonstrates its **defined behavior** under a suitable controlled test case, without implying unrealistic guaranteed success in stochastic runs.

### M4 — Metrics, detection and mitigation
**Build:** metrics collector, reward/economic accounting, export formats, fixed detector, adaptive detector, alerts and attack-specific defenses. Define observation windows and ground truth. Measure defensive overhead and side effects.

**Tests:** known synthetic detector labels and confusion counts, attack-specific defense on/off comparisons, metric consistency, correct JSON/CSV format.

**Done when:** baseline, attacked and protected results are directly comparable under the same declared model.

### M5 — Reproducible scientific campaigns
**Build:** scenario sweeps (hash power, node count/topology, latency, pool properties, seeds), automated run driver and aggregate outputs.

**Tests:** repeated identical inputs produce identical modeled results; different seeds vary where the model is stochastic; malformed configurations fail safely.

**Done when:** documented CLI commands run at least one complete three-condition experiment and produce machine-readable artifacts.

### M6 — Python analysis and report figures
**Build:** `analysis/run_campaign.py`, `aggregate.py`, `detection_eval.py`, `plots.py`; declare necessary Python packages in `requirements.txt` at this stage.

**Outputs:** summaries, uncertainty estimates where appropriate, confusion matrices, charts and limitations. ML is an optional extension after reliable baseline detectors.

**Done when:** graphs and tables regenerate from saved raw experiment results, not hard-coded numbers.

### M7 — Web interface and academic report
**Build:** FastAPI adapter to invoke the standalone C++ CLI and return structured outputs; lightweight frontend with the agreed Retro Web 1998 aesthetic. Ensure subprocess invocation and file inputs are validated. Prepare the report in the format required by the professor.

**Done when:** the same configured simulation can run from CLI and from web without altering core algorithms, and the interface shows the actual generated data.

## 6. Initial coding task for Codex

**Scope:** perform **M0 and the smallest runnable part of M1** only.

1. Read `projet.md`, `exo4.cpp`, `AGENTS.md`, and `PLAN.md`.
2. State any discrepancies between the assignment and this plan; do not silently reinterpret the source material.
3. Create a minimal root `CMakeLists.txt`, `src/main.cpp`, and just the necessary `src/core/` `.hpp/.cpp` files for an executable blockchain validation demonstration.
4. Preserve `exo4.cpp` without changes. Include meaningful CTest tests.
5. Configure, build and test in the current environment **if possible**; report exact commands and results, or the explicit blockers.
6. Update `README.md` with build/run instructions and `docs/DEVELOPMENT_LOG.md` with implemented/verified/remaining items.

**Do not:** implement network, attacks, detectors, machine learning, backend, frontend, dashboard, Docker or scientific report in the first coding task.

## 7. Build, test and output contract (to finalize in M1–M2)

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
# Example target CLI; actual arguments to be fixed when implemented:
./build/blockchain_sim --config configs/baseline.json --output results/raw/example
```

Plan for stable JSON configs and CSV/JSON outputs. Include human-readable errors and nonzero exit codes. Exact field schema, dependencies and platform-specific binary paths are to be documented when code exists, rather than assumed beforehand.

## 8. Important decisions and unresolved details

**Decided:** Claude-inspired modular structure; C++17/CMake/OpenSSL; reproducible discrete events; four required attacks; statistics in Python later; FastAPI + Retro Web 1998 frontend later; preserve original Atelier 1 code; incremental tests.

**Needs confirmation against `projet.md`:** exact deliverables and deadlines, mandatory consensus/PoS scope, experimental parameter ranges, grading rubric, expected GUI/report format, and mandated statistics. The plan deliberately does not invent these.

**Engineering choices to resolve during implementation:** C++ JSON library, tests/framework, compiler target, precise PoW event model, reward model, topology/latency model, exact metric definitions, detector ground-truth granularity, and operating-system compatibility.

**Resource constraints:** 8 GB RAM and limited free disk were reported. Keep the C++ build incremental and use out-of-source build folders; do not commit build outputs. Reassess local disk space before running large builds or installing additional tools.
