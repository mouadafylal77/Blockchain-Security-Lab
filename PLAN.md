# PLAN.md — Blockchain Security Lab

## Scope and authority

The academic `projet.md` mentions PoW/PoS. The owner now explicitly selects **PoW only** for the four-attack laboratory; no distributed PoS coverage is claimed. Preserve `exo4.cpp` unchanged as the original isolated PoW/PoS demonstration. No web, FastAPI, frontend or Machine Learning.

## Historical milestones retained

M0: requirements audit. M1: modular C++17/OpenSSL PoW core. M2: seeded discrete events, P2P, local block trees, cumulative work, forks/reorganizations. M3: four simplified attacks. M4: fixed/adaptive statistics and operational defenses (44 historical tests). M5: immutable reference of 720 evaluations, four calibrations, three pilot executions, tables, figures and report. Broader parameter sweeps remain future work. M6 analysis is partly delivered by M5. The former M7 web objective is superseded by the terminal; the academic report remains required.

## Architecture

Keep `src/core/` (PoW validation/tree), `src/sim/` (events/RNG/network/nodes/pools), `src/attacks/`, `src/detection/`, `src/mitigation/`, `src/metrics/`. Add only working `src/terminal/` components for scenario generation, stream-based input and observation. Keep `main.cpp` minimal. Configurations flow through the existing validated JSON contract. Add new examples under `configs/interactive/`; historical configurations and M5 analysis/results stay unchanged.

## Current vertical slices

1. R0 preservation: archive the frozen engine inputs, binary and runtime DLLs in ignored `build/m5-reference`, and verify all historical result hashes before changes. The historical M5 checker remains attached to that reference tree.
2. R1 terminal: `--interactive`, simple/custom modes, automatic seeded profiles, contextual attack inputs, validation/back/cancel, normalized JSON export and fresh output directories. Preserve old demo/config CLI.
3. R2 execution/observation: display discoveries, propagation, forks/reorganizations, private pedagogical state, alerts/actions and final outcomes. Target height belongs to an explicit local observer, distinct from total discoveries. Close mining at target (or after the explicitly configured post_target_ticks observation window) and drain pending deliveries under hard limits. Do not force private publication. Censored/non-accepted outcomes are not defensive successes.
4. R3 merchant policy: independent hold and heightened-confirmation lifetimes, optional public event-triggered audits. Preserve PoW fork choice and other defenses. Test early/late reactions, honest delays and persistent non-acceptance. Do not tune the attack to make protection win. New scenarios use a propagated public acceptance notice instead of the explicitly documented legacy merchant oracle.
5. R4 verification: existing/new CTest via `scripts/test-ucrt64.ps1`, GCC/OpenSSL UCRT64, CMake/Ninja/C++17, maximum two compilation tasks. Prepare a distinct multi-power evaluation with disjoint validation seeds; never replace M5.

## Scientific acceptance

Exponential block discovery follows relative resources; identities create no work. Local validation and greatest cumulative work remain authoritative, with deterministic hash ties. Distinguish stale, pending-parent and invalid. Seeded generation must not select results. One coordinated strategy per run; baseline retains all resources as honest participants. Shares differ from full blocks. Document Sybil bootstrap/admission assumptions. Detectors consume public observations only; honest calibration precedes attacked observations. Finite confirmations do not guarantee safety against a majority. Keep simulated and real time separate; document censoring and economic proxies. No commit or push without owner authorization.


## Terminal refinement 0.5.1 (owner request)

Preserve the existing terminal; add sectioned prompts with on-demand help, explicit simple-mode adversary power/pool membership or Sybil identities/network, a review/correction screen, and summary/detailed read-only rendering. Examine all four attacks in bounded paired terminal sessions, not a new M5 campaign. An optional post_target_ticks window continues ordinary production before bounded drainage; cancel inactive timers without advancing virtual time. Export production/drain/final-observation alert contexts and separate outcome from execution-limit status. Do not tune statistical thresholds or attack strategies to beautify outcomes. Report all limitations and honest side effects. Details and measured evidence: analysis/TERMINAL_REVIEW_051.md.


## M4 scientific refinement 0.6.0

Prospective hypotheses and acceptance criteria: docs/SECURITY_REFINEMENT_PLAN.md. Optional bounded renewed merchant holds and public decision ledger; context-bound/stratified honest references with explicit abstention; honest/adverse cost accounting after simulation; optional public bootstrap restrictions/delay. Preserve M1/M3 source and old configurations, default policy behavior and frozen M5. Targeted checks only; results and limitations in analysis/M4_REFINEMENT_REPORT.md. The separate benchmark proposal requires owner validation and measured pilot resource estimates before any large execution.
