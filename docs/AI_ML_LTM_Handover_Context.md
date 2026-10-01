# Release 20 AI/ML-assisted LTM: standards handover

## Status and source

This handover summarizes the study in [3GPP TR 38.745 V20.0.0, *Study on
Artificial Intelligence (AI)/Machine Learning (ML) for NG-RAN Phase 3*,
Release 20 (March 2026)](../standards/references/38745-k00.pdf). Page references
below are the printed 3GPP page numbers.

TR 38.745 is a **technical report**, not an implementation specification. Its
cover says it has not been approved for implementation and is provided for
future 3GPP work; clause 5 recommends the studied use cases as a baseline for
the Rel-20 normative phase. The report describes study scope, possible
solutions, candidate information and potential standard impacts. It does not,
by itself, make the AI/ML functions or the candidate information normative.
The report points to TS 38.300 for LTM and TS 38.401 for intra-CU LTM
(clause 4.2.1, printed p. 6).

## What the report studies

### Multi-hop UE trajectory

The Rel-18 cell-based UE trajectory prediction and measurement described in
the report are limited to the first-hop target NG-RAN node. The Rel-20 study
considers predicted and measured trajectories as chronologically ordered lists
of cells across gNBs (clause 4.1.1, p. 5).

Candidate training/inference placements are OAM training with gNB inference,
or training and inference in the gNB; CU-DU variants place inference, or both
training and inference, in the gNB-CU. Candidate inputs include serving- and
neighbour-cell measurements, UE mobility history, UE history from neighbouring
RAN nodes, and locally measured multi-hop trajectory. Candidate output is the
ordered predicted cell list with expected residence time per cell; measured
trajectory at visited gNBs is feedback (clauses 4.1.2.1–4.1.2.4, pp. 5–6).

The study describes transferring the prediction in Xn Handover Preparation,
and collecting/reporting subsequent measured trajectory through Data Collection
Reporting procedures (clause 4.1.2.5, p. 6). These are study solutions and
potential standard impacts, not claims that this report alone defines deployed
message formats or behavior.

### AI/ML-assisted intra-CU LTM

The study considers AI/ML optimization of intra-CU LTM, including L3- and
L1-measurement-based LTM with inference in the gNB-CU. Candidate placements are
OAM training with gNB-CU inference, or both in the gNB-CU (clauses 4.2.1 and
4.2.2.1, p. 6).

Candidate inputs include L3 results, UE history, measured/predicted per-cell or
SSB-area resource status and cell-based trajectory, historical candidate-cell
and beam lists, measured TA, UE reports and mobility history (clause 4.2.2.2,
p. 6). Candidate outputs include cells/beams for LTM preparation and cell
switch, cells/beams and TA for early UL synchronization, trigger timing for
L3-measurement-based LTM, the best beam for the first predicted trajectory
cell, and TA validity time (clause 4.2.2.3, p. 7). Feedback candidates include
the selected target cell/beam, measured TA, SON reports, and cell-switch and
early-UL-synchronization execution timing (clause 4.2.2.4, p. 7).

The report specifically leaves applicability of predicted TA validity time to
measured and/or predicted TA for assessment in the normative phase. It expects
F1 impacts, if applicable, to transfer the listed input, output and feedback
information (clause 4.2.2.5, p. 7).

### AI/ML-assisted inter-CU LTM

The report identifies candidate-cell selection as an example of AI/ML
optimization for inter-CU LTM. For potential solutions and impacts, it refers
to the intra-CU study where applicable, with standards impacts over Xn
(clauses 4.3.1–4.3.2, p. 7). It does not specify a detailed inter-CU model,
algorithm, or procedure.

## Boundaries: study facts versus implementation proposals

The following distinctions are important when using this handover:

| Topic | What TR 38.745 supports | Not established by this report |
| --- | --- | --- |
| Scope | Multi-hop UE trajectory, AI/ML-assisted intra-CU LTM, and AI/ML-assisted inter-CU LTM are recommended for Rel-20 normative work (clauses 1 and 5, pp. 4, 8). | A completed normative AI/ML feature or a deployed implementation. |
| LTM procedures | LTM and intra-CU LTM are specified by the referenced TS 38.300 and TS 38.401 (clause 4.2.1, p. 6). | That AI/ML replaces or controls existing standardized procedures. |
| AI outputs | Candidate cells/beams, TA-related values and certain timing predictions are studied for intra-CU LTM (clause 4.2.2.3, p. 7). | Direct AI control of MAC CEs, TCI state, a MAC scheduler, or packet forwarding. |
| Performance | The report lists use cases and candidate solutions. | Any numeric handover-interruption-time (HIT) gain, “~0 ms” result, guaranteed sub-10-ms result, or elimination of Time-To-Trigger (TTT). |
| Terminology | The report refers to gNB/gNB-CU, OAM, F1, Xn and the listed mobility data. | A “UPC Scheduler” or other non-standard architecture invented for this document. |

TR 38.745 contains no HIT ranges or claim that AI/ML eliminates TTT. Any
performance values or mechanisms proposed by an implementation must therefore
be identified as separately sourced experimental results, not attributed to
this report.

## Experimental implementation in this repository

The accompanying Python package is an **illustrative, local experiment**, not
an implementation of TR 38.745 or a radio-protocol stack. It trains a small
classifier on deterministic synthetic features and emits an advisory
recommendation for demonstration and testing. Its feature definitions,
synthetic labels, model, decision threshold and reported metrics are project
choices; they are not 3GPP requirements or radio-performance evidence. The
prototype does not generate or send RRC/MAC messages, operate a MAC/UPC
scheduler, or claim a handover-time improvement.

See the [main README](../README.md) for setup, tests, execution and the
repository layout. The longer [experimental design note](AI_ML_Mobility.md)
contains additional implementation proposals; read those as design material,
not normative TR 38.745 text.
