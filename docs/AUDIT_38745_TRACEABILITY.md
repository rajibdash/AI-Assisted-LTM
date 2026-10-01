# Audit: code vs. 3GPP TR 38.745 V20.0.0

**Source:** [`standards/references/38745-k00.pdf`](../standards/references/38745-k00.pdf)
(3GPP TR 38.745 V20.0.0, *Study on AI/ML for NG-RAN Phase 3*, Release 20,
March 2026). Page numbers below are the printed 3GPP page numbers (as shown
in the PDF footer), not PDF viewer page indices.

**Status of this note:** this is a repository-local engineering audit, not
part of TR 38.745. It records which study concepts this repository's
synthetic experiment represents in code, which it intentionally omits, and
why. It supersedes no normative text and creates no normative requirement.

## Why this note exists

An earlier revision of this repository implemented only a two-feature
heuristic (`neighbor_margin_db`, `target_load`) in `HandoverScorer`. On
review, several concepts described in TR 38.745 clause 4 — multi-hop
trajectory, model-placement options, candidate output/feedback structure —
were not represented at all. This note documents the resulting code changes
and the boundary between "represented as a modeled concept" and "out of
scope."

## Report concepts represented in code

| TR 38.745 concept | Clause (printed p.) | Code representation | What is *not* claimed |
| --- | --- | --- | --- |
| Multi-hop UE trajectory: "a list of cells... listed in chronological order" | 4.1.1 (p. 5) | `ltm_native::TrajectoryWindow` / `ltm_agent.mobility_context.TrajectoryWindow`: an ordered sequence of `MobilityObservation` entries, each carrying a `cell_id` and a monotonically non-decreasing `sequence_index`. | No cell topology, no real RSRP/RSRQ/SINR measurements, no Xn transfer. The "list of cells" is a plain in-memory sequence used only to compute a trend feature. |
| Locations for AI/ML Model Training and Model Inference | 4.1.2.1 (p. 5), 4.2.2.1 (p. 6) | `ltm_native::ModelPlacement` enum (`OamTrainingGnbInference`, `GnbTrainingAndInference`), attached to every `CandidateRecommendation` as metadata and queryable via `LtmRecommender::model_placement()`. | Purely a descriptive tag stored alongside a result. No training or inference actually executes in OAM, a gNB, or a gNB-CU process; everything runs in one local function call. |
| Candidate input data (serving/neighbour measurements, UE history, trajectory) | 4.1.2.2 (p. 5), 4.2.2.2 (p. 6–7) | `MobilityObservation` carries the two existing synthetic features (`neighbor_margin_db`, `target_load`) plus `cell_id`; `TrajectoryWindow.average_target_load()` is a simple aggregate standing in for "measured/predicted radio resource status." | No real measurement report formats, no `UE History Information` IE, no SSB-area resource status model. |
| Candidate output data: candidate/target cell(s) | 4.1.2.3 (p. 6), 4.2.2.3 (p. 7) | `CandidateRecommendation.cell_id` plus `urgency`/`recommended` identify a single candidate cell per call. | No beam list, no TA value(s), no per-cell ranked list, no timing-trigger predictions (these are listed in the report but have no code counterpart here; see "Out of scope" below). |
| Feedback of multi-hop trajectory / intra-CU LTM ("measured trajectory", "LTM target cell and beam", "SON Reports") | 4.1.2.4 (p. 6), 4.2.2.4 (p. 7) | `FeedbackRecord{cell_id, outcome_accepted, observed_margin_db}` and `LtmRecommender::record_feedback`/`feedback_history()`: a local, in-memory, append-only record correlating a prior candidate with an observed outcome. | No SON report schema (RLF/SHR), no measured TA, no transport of feedback anywhere; it is a plain data structure held in process memory only. |
| Standards impact over F1/Xn ("transferring the information listed...") | 4.1.2.5 (p. 6), 4.2.2.5 (p. 7), 4.3.2 (p. 7) | Represented only as *naming* — `ModelPlacement` distinguishes "OAM vs. gNB(-CU)" placement, matching the shape of the placement options the report lists for F1/Xn-relevant splits. | No F1 or Xn message, procedure, information element, or interface is implemented, simulated, or named as such in code. |
| Recommended use cases for Rel-20 normative work (multi-hop trajectory, AI/ML-assisted intra-CU LTM, AI/ML-assisted inter-CU LTM) | 1 and 5 (p. 4, p. 8) | The three concepts map to, respectively: `TrajectoryWindow`/`recommend_window` (trajectory), `LtmRecommender.recommend`/`CandidateRecommendation` (intra-CU LTM candidate scoring), and the fact that `LtmRecommender` has no CU/gNB-identity coupling so the same API can be reused conceptually "across" two hypothetical CUs (inter-CU). | No actual inter-CU boundary, negotiation, or admission-control model exists; see "Out of scope." |

## Explicitly out of scope (and why)

These concepts are named in TR 38.745 but are **not** represented in code,
because representing them would either require inventing non-standard
protocol behavior or would not add executable value to a local, synthetic
experiment:

- **Beams and TA values** (clauses 4.2.2.2–4.2.2.4, p. 6–7): no beam
  identifier, beam list, or Timing-Advance value/validity-time model is
  implemented. Modeling TA realistically requires timing/propagation
  assumptions well beyond this repository's scope, and the report itself
  leaves TA-validity applicability for the normative phase.
- **Trigger-timing predictions** ("timing to trigger cell switch", "timing
  to trigger early UL synchronization", clause 4.2.2.3, p. 7): these are
  time-domain scheduling predictions tied to real radio timers; adding them
  without a timer/scheduler model would be guesswork, not a meaningful
  experiment.
- **SON reports (RLF, SHR)** (clause 4.2.2.4, p. 7): `FeedbackRecord` only
  models an accepted/rejected outcome plus an observed margin value. A
  realistic RLF/SHR schema is out of scope; adding empty placeholder fields
  would not be meaningful.
- **F1 and Xn interfaces/procedures** (clauses 4.1.2.5, 4.2.2.5, 4.3.2, p.
  6–7): explicitly not implemented anywhere in this repository (see
  [README.md](../README.md) and
  [`AI_ML_LTM_Handover_Context.md`](AI_ML_LTM_Handover_Context.md)).
  `ModelPlacement` only *names* the placement options the report associates
  with potential F1/Xn impacts; it carries no network behavior.
- **Ranked/multi-candidate cell lists**: the report allows "candidate
  cell(s)" (plural); this code's `CandidateRecommendation` returns one cell
  per call. A full ranked-list API was judged to add complexity without
  materially improving the demonstration of the Python↔C++ boundary, and can
  be built later by calling `recommend`/`recommend_window` once per
  candidate cell.
- **Inter-CU negotiation and partial-acceptance handling** (clause 7.3 of
  [`AI_ML_Mobility.md`](AI_ML_Mobility.md)): no CU identity, Xn admission
  response, or partial-candidate-acceptance logic exists; `LtmRecommender`
  has no concept of "source" or "target" CU at all.

## Mandatory warnings

- Values, weights (`margin_weight`, `load_weight`, `bias`), the trend-based
  urgency adjustment in `recommend_window`, the `confidence` formula, and
  all `reasons` strings are **experimental project choices made for this
  repository**, not 3GPP requirements, measured results, or report content.
- TR 38.745 is a study report; its cover states it "has not been subject to
  any approval process by the 3GPP Organizational Partners and shall not be
  implemented" (p. 1). Nothing in this repository should be read as
  implementing, controlling, or replacing any 3GPP-specified procedure
  (LTM: TS 38.300; intra-CU LTM: TS 38.401).
- No network call, RRC/MAC message, F1/Xn procedure, or telecom simulator is
  implemented anywhere in this codebase, including in the new
  `ltm_native::TrajectoryWindow` / `LtmRecommender` additions described
  here.

## Where this fits in the repository

See the [main README](../README.md) for layout, build and test commands,
[`AI_ML_LTM_Handover_Context.md`](AI_ML_LTM_Handover_Context.md) for the
section-by-section standards handover, and
[`AI_ML_Mobility.md`](AI_ML_Mobility.md) for the longer engineering design
proposal that originally motivated these use cases (read as design
material, not normative text).
