# AI-Assisted LTM

This repository brings together Release 20 standards-study context for
AI-assisted LTM and a separate, runnable software experiment using synthetic
data. The experiment provides advisory candidate scoring; it is not a 3GPP
implementation and does not implement or control radio procedures.

## Standards context

The source is [3GPP TR 38.745 V20.0.0 (March 2026)](standards/references/38745-k00.pdf).
It is a study report, not an implementation specification: the cover states
that it has not been approved for implementation. Clause 5 identifies
multi-hop UE trajectory, AI/ML-assisted intra-CU LTM, and AI/ML-assisted
inter-CU LTM as recommendations for Rel-20 normative work; this is standards
context, not a requirement implemented by this repository.

The report studies possible model placements, inputs/outputs/feedback and
potential F1/Xn impacts (clauses 4.1–4.3). It does not specify numeric HIT
improvements, elimination of TTT, direct AI control of MAC CEs or TCI, or a
“UPC Scheduler.” Existing LTM and intra-CU LTM are referenced to TS 38.300 and
TS 38.401 respectively; see the [standards handover](docs/AI_ML_LTM_Handover_Context.md)
for details and page references. The [experimental design note](docs/AI_ML_Mobility.md)
contains proposals that must not be read as normative requirements. The
[audit/traceability note](docs/AUDIT_38745_TRACEABILITY.md) maps which of
these concepts are represented in code, which are intentionally out of
scope and why, with exact clause/page references.

## Presenter deck

The repository includes a Marp Markdown source for the Rel-20 technical
presentation:

- [AI-Assisted LTM Rel-20 — Presenter Edition](docs/AI_Assisted_LTM_Rel20_Presenter_Edition.md)

The source covers the proposed AI/ML-assisted candidate-cell ranking,
confidence-based Top-1/Top-K preparation policy, CU/L3/MAC/UPC/L1 impacts,
AI/ML interface contract, MLOps lifecycle, KPI framework, validation matrix,
and rollout guardrails. It is a design proposal and presentation artifact;
it does not add a normative 3GPP implementation to this repository.

A `.pptx` export is not committed. To generate one locally, install the Marp
CLI and run:

```sh
npm install -g @marp-team/marp-cli
marp docs/AI_Assisted_LTM_Rel20_Presenter_Edition.md \\
  --pptx \\
  -o docs/AI_Assisted_LTM_Rel20_Presenter_Edition.pptx
```

## Repository layout

```text
.
├── .gitignore
├── config/
│   └── default.json
├── docs/
│   ├── AI_Assisted_LTM_Rel20_Presenter_Edition.md
│   ├── AI_ML_LTM_Handover_Context.md
│   ├── AI_ML_Mobility.md
│   └── AUDIT_38745_TRACEABILITY.md
├── standards/
│   └── references/
│       ├── 38321-j40_MAC Spec.pdf
│       ├── 38745-k00.pdf
│       ├── AI_ML_LTM_Handover_Analysis.pdf
│       ├── Evolution_of_Layer1Layer2-Triggered_Mobility_in_5G.pdf
│       └── reducing-handover-interruption-l1l2-triggered-mobility.pdf
├── native/
│   ├── CMakeLists.txt
│   ├── include/ltm_native/
│   │   ├── mobility_score.hpp
│   │   └── trajectory.hpp
│   ├── src/
│   │   ├── bindings.cpp
│   │   ├── mobility_score.cpp
│   │   └── trajectory.cpp
│   └── tests/
│       ├── test_mobility_score.cpp
│       └── test_trajectory.cpp
├── src/ltm_agent/
│   ├── __init__.py
│   ├── __main__.py
│   ├── agent.py
│   ├── cli.py
│   ├── config.py
│   ├── data.py
│   ├── evaluation.py
│   ├── mobility_context.py
│   ├── model.py
│   ├── native_scorer.py
│   └── training.py
├── tests/
│   ├── test_ltm_agent.py
│   ├── test_mobility_context.py
│   └── test_native_scorer.py
├── LICENSE
├── pyproject.toml
├── setup.py
└── README.md
```

The PDFs are retained as source/reference artifacts. The Python package is an
isolated experiment; synthetic features and labels are illustrative project
choices, not standardized inputs or evidence of radio performance.

## Setup

Python 3.10 or newer is required. The runtime uses only the Python standard
library. The package build uses setuptools and pybind11.

```sh
python -m venv .venv
. .venv/bin/activate
python -m pip install -e .
```

On Windows PowerShell, activate with `.venv\Scripts\Activate.ps1`.

Installing in editable mode also compiles the `ltm_native` C++ extension
described below (a C++17 compiler is required; `pybind11` is installed
automatically as a build dependency).

## Test

The tests use Python's built-in `unittest` runner:

```sh
python -m unittest discover -s tests -v
```

## Python↔C++ binding (`ltm_native`)

`native/` contains two layers of C++ business logic exposed to Python as the
single `ltm_native` extension module via
[pybind11](https://github.com/pybind/pybind11):

1. A heuristic "handover-candidate" pre-filter (`ltm_native::HandoverScorer`,
   in `mobility_score.{hpp,cpp}`). It uses the same two mobility features as
   the rest of the experiment (`neighbor_margin_db`, `target_load`) and the
   same validation rules, but is a separate, explicit heuristic — not the
   trained model in `src/ltm_agent/model.py`.
2. A richer trajectory/recommendation domain model
   (`ltm_native::MobilityObservation`, `TrajectoryWindow`, `ModelPlacement`,
   `CandidateRecommendation`, `FeedbackRecord`, `LtmRecommender`, in
   `trajectory.{hpp,cpp}`) that models a few more TR 38.745 concepts: a
   simplified multi-hop trajectory (an ordered window of per-cell
   observations), AI/ML model-placement metadata, a structured recommendation
   with decision `reasons` and `confidence`, and a local, in-memory feedback
   history. See [`docs/AUDIT_38745_TRACEABILITY.md`](docs/AUDIT_38745_TRACEABILITY.md)
   for the exact clause/page mapping and what is intentionally out of scope.

Like the rest of this repository, both layers are advisory-only and
implement no 3GPP radio procedure.

```text
native/
├── CMakeLists.txt              # native-only build for the C++ unit tests
├── include/ltm_native/
│   ├── mobility_score.hpp      # public C++ interface (HandoverScorer, validation)
│   └── trajectory.hpp          # observation/window/recommender domain model
├── src/
│   ├── mobility_score.cpp      # scorer implementation
│   ├── trajectory.cpp          # domain model implementation
│   └── bindings.cpp            # thin pybind11 adapter -> `ltm_native` module
└── tests/
    ├── test_mobility_score.cpp # native C++ tests for HandoverScorer
    └── test_trajectory.cpp     # native C++ tests for the domain model
```

`src/ltm_agent/native_scorer.py` wraps `HandoverScorer` and exposes
`native_recommend(features, threshold=0.5)`. `src/ltm_agent/mobility_context.py`
wraps the trajectory/recommender domain model (`MobilityObservation`,
`TrajectoryWindow`, `LtmRecommender`, `FeedbackRecord`, `ModelPlacement`,
`CandidateRecommendation`). Its plain dataclasses validate and aggregate data
without the extension; calling the native scorer or using `LtmRecommender`
requires the extension and raises a clear `ImportError` if it is unavailable.
Python-level success, boundary, malformed-input, feedback, and
unavailable-extension behavior is covered by `tests/test_native_scorer.py`
and `tests/test_mobility_context.py`.

```sh
# Build and use from Python (also done by `pip install -e .`):
python -m pip install -e .
python -c "import ltm_native; print(ltm_native.HandoverScorer().evaluate(8.0, 0.2))"
python -c "
from ltm_agent import mobility_context as mc
recommender = mc.LtmRecommender()
print(recommender.recommend(mc.MobilityObservation('cell-1', 8.0, 0.2, 0)))
"

# Run only the native C++ tests, independent of Python:
cmake -S native -B native/build
cmake --build native/build
ctest --test-dir native/build --output-on-failure
```

## Run the local training/inference/evaluation demo

From the repository root, run:

```sh
ltm-agent demo --config config/default.json
```

Or, without installing the console command:

```sh
PYTHONPATH=src python -m ltm_agent demo --config config/default.json
```

The command generates deterministic toy data from the configured seed, splits
it into training and test sets, trains a small logistic-regression model,
reports test accuracy, and emits one advisory recommendation as JSON. Edit
`config/default.json` to change the sample count, seed, training fraction,
training settings, or decision threshold. The synthetic labeling rule and
resulting metrics are for exercising the pipeline only; they are not claims
about handover or HIT performance. No network, external service, telecom
simulator, secrets, or radio stack is used.
