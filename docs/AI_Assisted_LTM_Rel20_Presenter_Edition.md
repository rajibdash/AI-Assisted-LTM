---
marp: true
theme: default
paginate: true
size: 16:9
title: AI-Assisted LTM Rel-20 — Presenter Edition
description: Technical deep dive for reducing handover interruption time beyond Rel-18/19 baseline
---

<!--
Speaker notes:
Introduce the deck as a Rel-20 design proposal for reducing handover interruption time (HOIT) using AI/ML-assisted candidate-cell selection. The repository description identifies AI/ML Ops as the mechanism for improving handover or mobility scenarios. The repository is primarily C++ and Python, suggesting runtime integration plus data/model tooling.
-->

# AI-Assisted LTM (Rel-20)
## Reducing Handover Interruption Time Beyond Rel-18/19 Baseline

### Technical deep dive and software-design guide

**Repository:** `rajibdash/AI-Assisted-LTM`

- C++: 52.6%
- Python: 46.0%
- CMake: 1.4%

---

<!--
Speaker notes:
Explain the two main levers: improve target prediction and improve preparation strategy. Emphasize that AI is advisory and policy-controlled; the deterministic baseline remains available at all times.
-->

# 1. Executive Snapshot

## Problem
Rel-18/19 LTM behavior can experience higher HOIT under fast mobility, radio volatility, interference bursts, and target-cell load changes.

## Rel-20 proposal
- AI/ML-assisted candidate-cell ranking
- Policy-driven Top-1 or bounded Top-K preparation
- Readiness-aware handover command generation
- Fast fallback to the deterministic baseline

## Success target
Improve **P95/P99 HOIT** without regressing HO success rate, ping-pong rate, or resource efficiency.

---

<!--
Speaker notes:
Set expectations: this is an implementation-first flow from baseline gaps through interfaces, signaling, layer impacts, MLOps, validation, and rollout.
-->

# 2. Agenda

1. Rel-18/19 baseline and gap
2. Rel-20 target architecture
3. Top-1 and Top-K candidate selection
4. End-to-end signaling flows
5. CU, L3, MAC, UPC, and L1 impacts
6. AI/ML interface and MLOps
7. KPI, validation, and rollout
8. Software-designer checklist

---

<!--
Speaker notes:
Clarify that the baseline is not being replaced wholesale. The proposal adds prediction, bounded preparation, and observability around the existing mobility-control path.
-->

# 3. Rel-18/19 Baseline — Limitations

## Typical behavior
- Measurement-driven handover trigger
- Deterministic or heuristic target ranking
- Primarily single-target preparation
- Retry or reselection after late target failure

## Sources of interruption
- Candidate quality uncertainty
- Late preparation NACK or timeout
- Fast SINR/interference variation
- Target load changes after measurement collection
- Restarted procedures after a failed first choice

## Design implication
Optimize the tail: **P95/P99 HOIT**, not only the average.

---

<!--
Speaker notes:
Present this as a policy problem, not an AI-everywhere problem. High confidence should use the low-overhead Top-1 path. Uncertainty should activate bounded Top-K preparation. Low confidence or an expired deadline should use the baseline.
-->

# 4. Rel-20 Objective and Policy

## Design principles
- **Predict early** from time-windowed cross-layer data
- **Prepare smartly** according to confidence and risk
- **Switch decisively** only when readiness is adequate
- **Recover safely** using deterministic fallback

## Runtime policy
- `confidence >= Th_high` → Top-1
- `Th_low <= confidence < Th_high` → bounded Top-K
- `confidence < Th_low` or timeout → baseline

The CU remains the control authority; AI/ML provides a recommendation, confidence, and validity horizon.

---

<!--
Speaker notes:
Walk left to right. Measurements and scheduler/PHY statistics are collected, scored by the AI service, gated by policy, and passed to a preparation orchestrator. The fallback controller must be part of the normal architecture, not an afterthought.
-->

# 5. Condensed Rel-20 Architecture

```text
+------------------+       RRC / RAN signaling       +----------------------+
| UE               | <------------------------------> | Source CU / L3        |
| L1 / L2 / L3     |                                  | Mobility Controller   |
+---------+--------+                                  +----------+-----------+
          ^                                                      |
          | RF, PHY, MAC, UPC measurements                       | Inference req/resp
          |                                                      v
+---------+--------+                                  +----------------------+
| DU / L1 / MAC /  | <------ feature snapshots ------> | AI/ML Candidate Svc  |
| UPC schedulers   |                                  | rank + confidence    |
+------------------+                                  +----------+-----------+
                                                                  |
                                                                  v
                                                     +------------------------+
                                                     | Prep Orchestrator       |
                                                     | Top-1 / Top-K / fallback|
                                                     +------------------------+
```

---

<!--
Speaker notes:
Use these blocks to assign software ownership. The feature collector and observability layer should be treated as first-class components because they are needed for model quality, troubleshooting, and KPI attribution.
-->

# 6. Functional Building Blocks

- **Feature Collector**
  - L1/L2/L3 measurements
  - MAC/UPC load and buffer state
  - Mobility history and failure signatures
- **Inference Client**
  - Asynchronous request with deadline and cancellation
- **AI/ML Candidate Service**
  - Ranking, confidence, validity, reason tags
- **Policy Gate**
  - Confidence, risk, load, overhead, and freshness checks
- **Preparation Orchestrator**
  - Top-1 or bounded Top-K transaction handling
- **Fallback Controller**
  - Deterministic Rel-18/19 path
- **Observability Layer**
  - `hoTxnId`, model version, decision reason, timers, outcome

---

<!--
Speaker notes:
Top-1 minimizes overhead. Top-K is not an unrestricted broadcast or preparation of every neighbor. It is a bounded, policy-controlled hedge against late target failure.
-->

# 7. Candidate Strategy — Top-1 vs Top-K

| Mode | When to use | Advantage | Cost |
|---|---|---|---|
| Top-1 | Stable RF, high confidence, low risk | Lowest overhead and simplest flow | Sensitive to late target failure |
| Top-K | Medium confidence or high mobility risk | Better resilience and faster fallback | Extra preparation and cleanup |
| Baseline | Low confidence, stale data, timeout | Known-safe behavior | No AI-assisted gain |

## Runtime selector
`mode = f(confidence, mobility_risk, cell_load, latency_budget, data_freshness)`

---

<!--
Speaker notes:
Break HOIT into measurable stages and map each stage to owners. The AI service must not consume the complete control-loop budget. Preparation readiness and recovery are often more important than raw model accuracy.
-->

# 8. HOIT Timing Budget

## Measurable contributors
1. Measurement and trigger detection
2. Feature assembly and inference
3. Target preparation signaling
4. HO command and access/synchronization
5. Path switch and resource cleanup
6. Recovery after target failure

## Rel-20 optimization points
- Low-latency local or near-edge inference
- Time-windowed feature cache
- Readiness-aware command generation
- Bounded Top-K preparation for high-risk cases
- Fast failure branch without full procedure restart

---

<!--
Speaker notes:
Use standards-aligned names where they match the deployment. The exact protocol realization can vary with the architecture. Internal interfaces should be defined independently of the external signaling choice.
-->

# 9. Signaling and Interface Naming

## External signaling families
- UE control: RRC-like `MeasurementReport`, reconfiguration, HO command
- Inter-node preparation: XnAP-like preparation and response
- Core path control: NGAP-like context/path procedures where applicable
- CU/DU split: F1AP-like UE-context and configuration procedures

## Internal software interfaces
- `IAICandidateScoringClient`
- `ICandidatePreparationManager`
- `IHoFallbackController`
- `ISchedulerHoHintInterface`
- `IL1MeasurementFeaturePublisher`
- `IHoObservabilitySink`

---

<!--
Speaker notes:
This is the high-confidence fast path. Apply deadlines to inference and preparation. The command should be sent only when the selected target meets minimum readiness criteria.
-->

# 10. Signaling Flow A — Top-1 Fast Path

```text
UE              Source CU/L3          AI/ML Service          Target Cell
| MeasurementReport  |                     |                     |
|------------------->|                     |                     |
|                    | InferenceRequest   |                     |
|                    |-------------------->|                     |
|                    | InferenceResponse  |                     |
|                    |<--------------------|                     |
|                    | HO_Preparation_Req  |--------------------->|
|                    | HO_Preparation_Ack  |<---------------------|
| RRC_HO_Command     |                     |                     |
|<-------------------|                     |                     |
| Access / Sync      |------------------------------------------->|
|                    | HO_Complete / PathSwitch                  |
|                    |<-------------------------------------------|
|                    | Release source context                      |
```

## Required timers
- `t_feature_max`
- `t_infer_max`
- `t_prep_max`
- `t_command_max`
- `t_recovery_max`

---

<!--
Speaker notes:
Explain that Top-K should normally be limited to a small K, such as two active candidates, subject to deployment capacity. The command selects the best candidate that is both ranked well and ready.
-->

# 11. Signaling Flow B — Top-K Risk-Managed Path

```text
UE              Source CU/L3          AI/ML Service       Candidate B/C
| MeasurementReport  |                     |                    |
|------------------->|                     |                    |
|                    | InferenceRequest   |                    |
|                    |-------------------->|                    |
|                    | RankedList(K),Conf  |                    |
|                    |<--------------------|                    |
|                    | PrepReq(B) ------------------------------>|
|                    | PrepReq(C) ------------------------------>|
|                    | <--------- PrepAck / Nack / Timeout       |
|                    | select best ready candidate               |
| RRC_HO_Command     |                     |                    |
|<-------------------|                     |                    |
| Access / Sync ------------------------------------------------->|
|                    | HO_Complete / PathSwitch                   |
|                    |<------------------------------------------------|
|                    | cleanup non-selected contexts               |
```

## Guardrails
- `K_active <= configured_limit`
- Candidate load admission check
- Candidate freshness and readiness check
- Cleanup deadline for unused prepared context

---

<!--
Speaker notes:
Review each branch as a mandatory test case. Every AI failure must have a bounded outcome and a deterministic next action.
-->

# 12. Failure and Recovery Branches

| Condition | Immediate action | Longer-term action |
|---|---|---|
| Inference timeout | Use baseline decision | Track AI SLA violation |
| Stale feature window | Reject response | Tune collection/freshness policy |
| Top-1 prep NACK | Try ready backup or baseline | Update candidate failure statistics |
| Top-K partial failure | Select best ready candidate | Release unused contexts |
| Post-command failure | Fast recovery and cooldown | Blacklist or reduce target confidence |
| Model drift alarm | Disable AI control impact | Pin previous model/version |
| CU/AI service unavailable | Baseline path | Circuit-breaker recovery |

---

<!--
Speaker notes:
The CU owns orchestration correctness, transaction lifetime, deadlines, and correlation. Make all transitions idempotent and observable.
-->

# 13. CU Impact — Orchestration

## New or extended modules
- Mobility Decision Orchestrator FSM
- Asynchronous AI inference client
- Candidate Preparation Manager
- Fallback and Recovery Controller
- Timer and deadline manager
- Per-HO trace context

## Suggested FSM
`IDLE → EVAL → PREP → CMD_SENT → COMPLETE`

Failure transitions:
`EVAL → BASELINE`, `PREP → RECOVER`, `CMD_SENT → RECOVER`

## Required properties
- Idempotent retries
- Cancellation on stale transaction
- Correlation using `hoTxnId`
- Bounded state lifetime

---

<!--
Speaker notes:
L3 owns the mobility policy and decides when an AI response is acceptable. Thresholds must be configuration-driven and support safe rollback.
-->

# 14. L3 Impact — Mobility Decision Logic

## Responsibilities
- Convert measurement reports into time-windowed features
- Enforce candidate eligibility and policy constraints
- Apply confidence/risk thresholds
- Gate HO command on target readiness
- Emit decision reason codes

## Example reason codes
- `AI_TOP1_HIGH_CONF`
- `AI_TOPK_RISK_MITIGATION`
- `FALLBACK_LOW_CONF`
- `FALLBACK_TIMEOUT`
- `FALLBACK_STALE_FEATURES`
- `TARGET_PREP_NOT_READY`

## Configuration
Thresholds, K limit, freshness window, timers, and load caps must be remotely tunable with versioning.

---

<!--
Speaker notes:
MAC should expose both input features and control hooks. The goal is to reduce residual buffered data and avoid scheduling starvation near the switch window.
-->

# 15. MAC Scheduler Impact

## Required behavior
- Mark UE as HO-pending with an estimated switch time
- Prioritize control and handover-critical bearers
- Reduce residual buffer before the command where appropriate
- Export cell load, PRB pressure, queue, and grant-delay features
- Accept target-preparation hints without violating fairness

## Suggested interfaces
```text
onHoPending(ueId, etaSwitch)
getCellLoadSnapshot(cellId)
getUeBufferState(ueId)
getGrantDelay(ueId)
clearHoPending(ueId, outcome)
```

## KPIs
- Residual buffer at HO command
- Grant delay around HO window
- Scheduler fairness and CPU impact

---

<!--
Speaker notes:
UPC scheduling focuses on user-plane continuity. Duplication must be short-lived and bounded. Path switch and cleanup need clear ownership to avoid leaks or stale forwarding.
-->

# 16. UPC Scheduler Impact

## Required behavior
- Classify HO-critical flows
- Apply bounded temporary duplication or forwarding where supported
- Commit target path quickly after target confirmation
- Tune reorder guard window for the expected interruption
- Drain and release the old path promptly

## Suggested interfaces
```text
armHoDuplication(ueId, durationMs)
commitPathSwitch(ueId, targetPath)
setReorderGuard(ueId, windowMs)
drainAndReleaseOldPath(ueId)
```

## KPIs
- Burst packet loss
- Packet reordering
- Throughput recovery time
- Duplicate-packet overhead

---

<!--
Speaker notes:
L1 provides the most time-sensitive radio evidence. Trends and variance should be exported, not only instantaneous samples. Readiness indications help the CU avoid sending a command to a target that is not ready.
-->

# 17. L1 Impact — Measurement and Readiness

## Time-windowed feature examples
- `sinr_mean`, `sinr_slope`, `sinr_variance`
- `bler_mean`, `bler_spike`
- RSRP/RSRQ trend and variance
- Timing/frequency stability
- Beam or synchronization readiness
- Measurement age and quality flags

## Execution hooks
- Fast readiness indication to CU
- Fast candidate-failure indication
- Timestamped samples for end-to-end budget accounting
- Schema version and validity interval on every feature set

---

<!--
Speaker notes:
Treat this as a versioned contract. The CU must validate response freshness, candidate eligibility, and latency before applying the response.
-->

# 18. AI/ML Interface Contract

## Request: CU → AI/ML service
```json
{
  "schemaVersion": "1.0",
  "hoTxnId": "opaque-transaction-id",
  "ueId": "opaque-ue-id",
  "servingCell": "cell-A",
  "candidateCells": ["cell-B", "cell-C", "cell-D"],
  "featureWindow": {
    "startMs": 0,
    "endMs": 200,
    "features": {}
  },
  "policyContext": {
    "maxK": 2,
    "latencyBudgetMs": 20,
    "modelVersion": "configured"
  }
}
```

## Response: AI/ML service → CU
```json
{
  "schemaVersion": "1.0",
  "hoTxnId": "opaque-transaction-id",
  "rankedCandidates": [
    {"cell": "cell-B", "score": 0.91, "confidence": 0.88},
    {"cell": "cell-C", "score": 0.84, "confidence": 0.79}
  ],
  "modeHint": "TOP1",
  "validityMs": 50,
  "reasonTags": ["rf_stable", "load_ok"]
}
```

---

<!--
Speaker notes:
The model should use trends, history, and load context. Define labels around both immediate handover success and post-handover stability so the model does not optimize only for command completion.
-->

# 19. Data and Model Strategy

## Feature groups
- RF level and trend features
- SINR/BLER volatility
- Mobility speed/class and trajectory proxy
- Serving and candidate load
- MAC queue and grant pressure
- Previous HO outcomes and failure causes
- Feature age and measurement confidence

## Labels
- Successful HO with low interruption
- Preparation failure or timeout
- Access/synchronization failure
- Re-attempt and ping-pong
- Stable post-HO quality window

## Model outputs
- Candidate ranking
- Calibrated confidence
- Validity horizon
- Reason tags for operations and debugging

---

<!--
Speaker notes:
Shadow mode is the first safety gate. Model and feature schema compatibility must be managed like software releases. Drift detection and rollback need automatic triggers.
-->

# 20. MLOps Lifecycle

## Offline
- Dataset quality and completeness checks
- Leakage prevention and time-based splits
- Class imbalance and rare-failure handling
- Baseline comparison against Rel-18/19 heuristic logic
- Confidence calibration and threshold tuning

## Online
- Shadow inference with no control impact
- Canary activation by cluster, cell, UE class, or mobility class
- Model, schema, and feature-version telemetry
- Drift and data-quality alarms
- Automatic rollback or policy disablement

## C++ / Python / CMake mapping
- C++: low-latency runtime and control integration
- Python: training, feature engineering, evaluation, MLOps
- CMake: modular build targets and test binaries

---

<!--
Speaker notes:
Make P95/P99 HOIT the primary outcome. Every gain must be checked against success rate, ping-pong, signaling overhead, and resource overhead.
-->

# 21. KPI Framework and Acceptance Gates

## Primary KPIs
- HOIT: P50, P95, P99
- Handover success rate
- Re-attempt rate
- Ping-pong rate

## Secondary KPIs
- Packet-loss burst during handover
- Post-HO throughput recovery time
- Signaling overhead from Top-K
- CU/DU/scheduler CPU and memory
- AI inference latency and timeout rate

## Acceptance rule
No statistically significant regression in primary mobility KPIs. Improvements must be measured against a frozen Rel-18/19 baseline under comparable traffic and radio conditions.

---

<!--
Speaker notes:
Test both normal and adversarial conditions. Failure injection is mandatory because the value proposition is strongest in late-failure scenarios.
-->

# 22. Validation Matrix

| Dimension | Test conditions |
|---|---|
| Mobility | Pedestrian, vehicular, high-speed |
| RF | Stable, fading, interference burst |
| Load | Low, medium, congested |
| Topology | Macro edge, hotspot, mixed neighbor quality |
| Candidate quality | Strong, close-ranked, rapidly changing |
| Service state | AI available, delayed, unavailable, stale |
| Failure injection | Prep NACK, timeout, access failure, path-switch delay |

## Methods
- A/B comparison against baseline
- Shadow inference before control activation
- Replay of field traces
- Statistical significance and confidence intervals
- Soak and endurance testing

---

<!--
Speaker notes:
Roll out in phases. Each phase needs measurable exit criteria and a rollback path. Start with Top-1 because it is simpler to validate, then introduce adaptive Top-K where data shows value.
-->

# 23. Rollout Plan and Guardrails

## Phases
0. Instrumentation and data readiness
1. Shadow inference and observability
2. Top-1 guarded canary
3. Adaptive Top-K for high-risk cases
4. Wider rollout and continuous retraining

## Guardrails
- Per-cluster and per-cell policy override
- One-click AI disablement
- Model/version pinning
- Circuit breaker for inference service
- Automatic baseline fallback
- Incident dashboard keyed by `hoTxnId` and reason code

## Exit gate
KPI improvement, no mobility regression, bounded overhead, and tested rollback.

---

<!--
Speaker notes:
Use this slide to assign implementation ownership. The first release should deliver a safe Top-1 path, complete observability, and a robust baseline fallback before adding Top-K complexity.
-->

# 24. Software Designer Implementation Checklist

- [ ] CU FSM and transaction model with `hoTxnId`
- [ ] AI client with deadline, cancellation, circuit breaker
- [ ] L3 confidence/risk policy and fallback thresholds
- [ ] Candidate preparation manager and cleanup logic
- [ ] MAC HO-pending hints and buffer-drain strategy
- [ ] UPC duplication/path-switch/reordering controls
- [ ] L1 feature publisher and readiness indications
- [ ] Versioned AI request/response schemas
- [ ] Decision reason codes and end-to-end tracing
- [ ] Shadow, canary, A/B, and rollback controls
- [ ] KPI dashboards and failure-injection tests

## Recommendation
Start with **Top-1 + strong fallback**. Enable **adaptive Top-K** only after measured risk/benefit validation.

---

# Appendix A — Design Review Checklist

## Architecture readiness
- [ ] CU/L3/MAC/UPC/L1 boundaries documented
- [ ] AI service integration pattern approved
- [ ] End-to-end timing budget allocated
- [ ] Fallback architecture reviewed

## Interface contracts
- [ ] AI request/response schema versioned
- [ ] Reason-code taxonomy frozen
- [ ] `hoTxnId` propagation defined
- [ ] Backward compatibility documented

## CU/L3 gates
- [ ] FSM transitions reviewed for races and deadlocks
- [ ] Timeout and retry matrix approved
- [ ] Thresholds parameterized
- [ ] HO command readiness criteria defined

---

# Appendix B — Layer and MLOps Review Gates

## MAC / UPC / L1
- [ ] MAC prioritization policy validated
- [ ] UPC duplication and reordering limits approved
- [ ] L1 feature sampling/window specification approved
- [ ] Readiness/failure fast path verified

## MLOps and data
- [ ] Data quality checks automated
- [ ] Leakage and drift baselines documented
- [ ] Shadow telemetry complete
- [ ] Rollback thresholds defined

## KPI and validation
- [ ] Rel-18/19 baseline frozen
- [ ] A/B methodology approved
- [ ] Failure-injection coverage complete
- [ ] Non-regression guardrails defined

---

# Appendix C — Production Go/No-Go Checklist

- [ ] P95/P99 HOIT improvement is statistically significant
- [ ] HO success rate has no unacceptable regression
- [ ] Ping-pong and re-attempt rates are within limits
- [ ] Signaling overhead is within budget
- [ ] CU/DU/scheduler resource overhead is within budget
- [ ] Inference timeout rate is within budget
- [ ] Model and feature versions are traceable
- [ ] Canary and rollback procedures were tested
- [ ] Operations runbook and dashboards are live
- [ ] 30/60/90-day monitoring plan is approved

## Final design position
AI/ML should be introduced as a **bounded, observable recommendation service**. The CU/L3 mobility controller retains authority, and every AI path must have a deterministic, deadline-safe fallback.
