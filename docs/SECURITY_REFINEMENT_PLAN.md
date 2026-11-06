# M4 scientific refinement — hypotheses before implementation

Historical M5 and the current M1/M3 sources/configurations are preserved. No full campaign is authorized. Defaults retain existing behavior; new policy/admission options are explicit. Hypotheses below are prospective engineering choices, not claims of established effectiveness.

## Lot A — merchant causality and bounded payment hold

Hypothesis: independent confirmation/hold lifetimes already work; a finite cap on renewed holds can improve honest payment availability, at the possible cost of accepting more risky payments. Default cap=0 retains unlimited renewal. Zero hold is allowed for a confirmation-only policy. Existing public fork/silence warnings are hypotheses of risk, not proof of a private attack. Reorganization-only signals remain necessarily late for a first hidden reversal. No new attacker information or modified publication strategy.

Acceptance: public merchant decisions record local payment presence, base/effective confirmations, hold state and event sequence before M3 evaluates acceptance. Export actual accepted/reversed payment and the first base-eligible opportunity on the realized trajectory (not a causal no-defense counterfactual). Test independent expiry, capped renewals, repeated honest warnings, early and late intervention, non-acceptance/censoring and reproducibility. Costs: longer confirmation waits, honest holds/refusals, additional decision records. Paired honest runs are needed to estimate policy costs; refusing all payments is never success.

## Lot B — honest, context-bound network reference

Hypothesis: node count, topology, latencies, mining cadence and observation windows change honest metrics. A matched honest reference stratified by node/component/metric can reduce alarms from heterogeneous honest delays without silencing an unusual fast-peer delay. No arbitrary lowering of fixed thresholds. EWMA scoring-before-update/clipping remains unchanged.

Acceptance: optional stratification, honest-only reference export, profile SHA-256 excluding seed and defense activation but including public network/resources/measurement semantics. New bound references reject mismatched profiles; legacy references remain readable unless strict matching is requested. Missing/insufficient strata are counted and never invented. Test synthetic heterogeneous honest peers and a real anomaly, mismatched node/link/cadence profiles, seed-independent fingerprints, no future labels and independent honest validation. Costs: more reference cells and memory, more calibration runs, insufficient strata, possible slower detection. No nominal Gaussian false-positive guarantee or tuned benchmark claim. Measure remaining honest alert/intervention rates by run.

## Lot C — defense costs and limited bootstrap access

Hypothesis: probation and pool auditing can harm honest actors; an unrestricted one-tick bootstrap overstates Sybil recovery when independent peers are hard to reach. Keep selfish/pool policy rules unchanged. Add opt-in public bootstrap allowlist and connection delay (defaults: unrestricted/zero extra delay), without consulting adversarial identity labels.

Acceptance: export deferred sender vs producer roles post-run only, honest/adverse share credits/payouts, pool reward (paired loss requires honest reference), reconnect requests and first adoption after connection (not convergence/proven exclusive causality). Test preserved no-defense behavior, honest producer/relay costs, honest/adverse pool sanctions, bootstrap available/delayed/unavailable cases and null recovery when censored. Costs: messages, added ticks, false sanctions, lost availability, uncredited shares; accounting itself has storage/processing overhead. Run bounded paired examples, never M5 or a large sweep.

## Independent benchmark proposal

Define a new protocol after these lots, with disjoint honest-calibration/validation/final-test seeds, several powers and topologies, fixed horizon/budget across modes, four honest/attacked/protected/honest-protected controls, and run-level uncertainty. Calibration/validation must not use final tests. Large execution awaits owner approval. Publish failures/regressions as well as gains.
