# AGENTS.md — Blockchain Security Lab

## Mission
Develop **Blockchain Security Lab**, an academic, reproducible blockchain security simulation platform. Owner decision: PoW only in the four-attack laboratory, with a C++17 interactive terminal (simple/custom). No distributed PoS, web or Machine Learning. Disclose the PoW/PoS wording of the academic assignment; preserve its isolated Atelier 1 demonstration.

## Source of truth and project context
Read the files relevant to the task, in this order of authority:
1. `projet.md` — official academic assignment, if this is the faithful transcription of the professor's requirements. Requirements take precedence over architecture choices.
2. `AGENTS.md` — implementation and workflow rules.
3. `PLAN.md` — agreed architecture, milestones, scientific validation criteria.
4. `exo4.cpp` — original Atelier 1 reference implementation; **never modify this file**.

If `projet.md` is missing or incomplete, state precisely which requirements cannot be verified. Do not invent academic requirements or quietly override them. If documents conflict, report the conflict and follow the official assignment for mandatory behavior. `PLAN.md` is the authoritative architectural roadmap unless an explicit user decision changes it.

## Technology and conventions
- Core engine: **C++17**, **CMake**, **OpenSSL** for SHA-256; use a compatible C++ compiler.
- Prefer self-contained, modular `.hpp` / `.cpp` files, with a minimal `src/main.cpp`.
- Automated C++ tests via **CTest** (a lightweight test framework is optional).
- Read simulation configuration from **JSON**; export results as **CSV and/or JSON**, with a documented schema.
- Use a seeded random number generator, simulated time, and deterministic tie-breaking for reproducibility.
- Python is optional experimental analysis only. JSON supports replay/export; the terminal generates validated scenarios without manual JSON editing.
- Never require Python, FastAPI, or a frontend to build or execute the C++ engine.
- Avoid unnecessary heavyweight dependencies and build artifacts; do not commit generated binaries, large results, or local environment files.

## Architecture
Respect the target organization in `PLAN.md`:
- `src/core/`: blocks, transactions, blockchain and consensus.
- `src/sim/`: discrete-event engine, P2P network, nodes, miners and pools.
- `src/attacks/`: 51% double-spend/reorganization, selfish mining, block withholding, Sybil.
- `src/metrics/`: measurements and data exports.
- `src/detection/`: fixed/adaptive rule-based detection.
- `src/mitigation/`: configurable countermeasures.
- `configs/`, `tests/`, `analysis/`, `results/`, `scripts/`, `report/`: scenarios, verification and reporting.

Do **not** create the complete directory tree as empty scaffolding. Add real files as components are implemented and tested. Preserve `exo4.cpp` and refactor reusable concepts into new modules rather than copying unexamined assumptions.

## Scientific correctness
- Explicitly document simplifications and assumptions; do not claim to emulate real Bitcoin or Ethereum in full.
- For the PoW model, separate network node count from mining hash power; select the canonical chain by **cumulative work** under the documented model, not node majority.
- Distinguish **stale blocks** from **unknown-parent/orphan blocks** where relevant; define measurements clearly.
- A Sybil attack manipulates network identities/connectivity and does **not** automatically provide extra hash power.
- The simplified Atelier 1 PoS may be retained as an isolated teaching component; do not present it as Ethereum's consensus or assume PoW attack results transfer to PoS.
- Test each attack with meaningful behavioral assertions, not merely successful execution.
- Measure baseline, attacked and protected cases with controlled configurations and seeds. Never fabricate metrics, results or test outcomes.
- Explain limitations and trade-offs of mitigations; no defense is automatically safe or effective.

## Workflow — one vertical slice at a time
For each task:
1. Inspect the minimum relevant documents and existing code.
2. State the component to implement and its acceptance criteria; avoid unrequested architecture changes.
3. Implement the smallest coherent increment, keeping `main.cpp` minimal.
4. Add or update automated tests and sample configuration as needed.
5. Run configure/build/test commands **if the environment supports them**; report exact commands and pass/fail results. Never say tests passed unless they actually ran.
6. Update relevant documentation and a concise progress entry in `docs/DEVELOPMENT_LOG.md` (create it when useful).
7. Report what changed, what was verified, remaining limitations, and the next actionable step.

## Integration contract
Design the C++ engine as a standalone CLI, targeting a command similar to:

```bash
./blockchain_sim --config configs/baseline.json --output results/raw/run_001
```

The exact executable name and arguments must be finalized in implementation and documented in `README.md`. Errors should return a nonzero exit status. Outputs should identify the scenario, seed, parameters and relevant measurement definitions for replay by the standalone C++ terminal.

## Priority and constraints
Finish the **C++ engine and its tests first**, then experimental analysis, then the academic report. Keep local builds lightweight and incremental: the development machine has limited RAM and disk space. Favor clear, explainable code over premature features, machine learning, dashboards, Docker or report automation.

## Revised development contract (owner decision)
Before changing frozen inputs or rebuilding, run `scripts/preserve-m5.ps1 -Create`; verify with the same script without -Create. Preserve the local ignored archive `build/m5-reference` and all historical M5 data, configurations, analysis and manifests. Never rerun the original 720 cases to replace results. New evaluations use separate paths/model versions. Keep historical JSON compatibility.

Add `--interactive` with simple/custom modes, back/cancel and contextual inputs. Resource totals are sums of actual miners, never extra virtual hash power. Pedagogical private displays must not feed detectors/honest nodes. Distinguish failed, successful and inconclusive attacks; payment refusal is not successful protection. Separate merchant hold and strengthened-confirmation durations. Use public signals only. Keep builds at two tasks maximum. No commits/pushes without authorization.
