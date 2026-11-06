# Independent M4 benchmark — proposal, not executed

Owner approval is required before the large campaign. No historical M5 run, artifact or conclusion is replaced. This protocol must be frozen with software/binary/config hashes after the targeted lots are reviewed. Current targeted tests are development evidence, never final-test samples.

## Hypotheses and outcomes

1. A capped renewal of payment holds may improve honest acceptance/latency but may increase reversal risk. Compare no defense, uncapped and capped policies; do not call non-acceptance successful protection. Reorg-only detection cannot save a payment already accepted. Public fork/silence warnings are preventive hypotheses, not identification of a private chain.
2. Matched, node/peer-stratified honest references may reduce benign propagation alarms; this may miss attacks if strata are insufficient or calibration is unrepresentative. Retain fixed thresholds as baseline and publish all skipped strata/false interventions.
3. Probation/auditing can impose honest costs; Sybil recovery may fail when independent public bootstrap candidates are unavailable or delayed. Economic effects, honest traffic delayed and recovery delay matter independently of binary M3 success.

## Proposed blocks (16 profiles, no full factorial claim)

- 51%: powers 10/40/85% x 5-node ring/30-node ring (6).
- Selfish: powers 20/40%, 5-node ring (2).
- Pool: infiltrator 10/40%, 5-node ring, shares d-1/full blocks d=2 (2).
- Sybil: 5-node ring, independent directory available / additional connection delay250 / only controlled relays accessible (3). No added mining power.
- Three declared stress profiles: one per mining attack, q40%, 30-node degree-four graph, latency20+jitter0..10 (3).

Normal links: latency5+jitter0..2, global mean100 ticks (pool200). Positive individual mining weights sum to100, explicitly assigned before seed draws; same resource ownership across paired modes, honest baseline runs all resources honestly. Keep confirmation count6, duration20000 ticks and max_blocks100 fixed per profile; max_events/max_pending_events100000 are safety limits. No target-height stopping. Profiles with several changed network factors are stress blocks, not independent causal estimates of size vs latency.

Two primary detectors: fixed engineering thresholds unchanged; matched stratified EWMA. For 51%, six conditions per detector/seed: honest and attacked, each with no defense / uncapped / capped hold. Other attacks use honest/no defense, honest/protected, attacked/no defense, attacked/protected. A global-reference adaptive ablation may be added only before protocol freeze with explicit added cost, never chosen after seeing final outcomes.

## Data separation and parameter selection

Reserve disjoint ranges after checking all existing manifests: proposed calibration50001..50010, validation60001..60010 and final70001..70030. Two pilot seeds per profile are excluded from all three ranges. Honest calibration precedes evaluation; each reference records all contributing seeds and the exact public profile. The current single-run reference writer requires a tested reference-moment merge before claiming a multi-seed calibration; that merge is not silently simulated by pooling test data.

Validation grid (prospective): alpha0.05/0.1, k3/4; hold500, strengthened lifetime2000, cap0/1000. Use validation only to select one declared matched-reference setting per profile, retaining uncapped as comparison. Do not select only by attack success: report interventions on honest participants, acceptance fraction, honest latency and losses. Proposed operational tolerance 5% of honest runs with an intervention is a discussion target, not a proven bound or owner-approved requirement. Thirty final seeds have limited precision for rare harm; zero observed harm is not proof of a sub-5% risk. If no setting meets the agreed trade-off, retain and publish that failure.

No future data, private branches or attack labels enter thresholding/policy decisions. Document contamination clipping and missing strata. After final results are opened, a parameter change requires a new version and new final seeds.

## Measurement and inference

Keep M4 run x primary-family confusion counts for comparability, explicitly as association rather than per-event attack attribution. Add merchant-local action timing, accepted-and-reversed fraction, accepted honest fraction, non-acceptance/censoring, first base-ready opportunity on the realized trajectory, and paired actual acceptance-time differences when both accept. Same-tick sequence/trace order determines causality; tick-only 'strictly before' is conservative. Global post-reversal warnings do not count as successful prevention.

For selfish: active reward share, honest-sender vs honest-producer delayed messages, unique affected payloads and added simulated delay. These categories overlap; never sum them as disjoint victims. For pools: credited/uncredited honest/adverse shares, actual pool reward/payout units and paired losses versus honest control, not 'withheld solutions = exact lost rewards'. For Sybil: request, first connection, first adoption, censored/no recovery and overhead; adoption after reconnection is not proof of exclusive sync provenance or full convergence.

Pair by seed; bootstrap whole seed clusters, not correlated node/window observations. Publish n, missing/censored values, distributions and intervals; include Wilson intervals for boundary proportions. Simulated messages/ticks are separate from process CPU/RSS. Preserve all failed executions and outputs; no seed replacement or repeated draws until a desired outcome.

## Resource and execution gate

Nominal final size: 7 double-spend profiles x2 detectors x6 conditions x30 seeds =2520; 9 other profiles x2 x4 x30 =2160, total4680 final runs. Training160 runs, validation1560 plus32 pilots: nominal6432, before any declared ablations. This is a proposal, not a resource guarantee. Measure wall time and disk on the 32 pilots first, project storage conservatively, and agree an explicit disk budget before launch. Run sequentially, compiler parallelism<=2. If the budget is too large, approve a smaller preregistered block list before final seeds, never remove profiles because results are unfavorable. Current exports retain full traces; no compact-export feature or unmeasured storage reduction is assumed.

Required approval: profile list/resources, false-intervention tolerance, policy/validation grid, any multi-seed reference-merge implementation, final seeds, and measured disk/time budget. No large runner is launched by this proposal.
