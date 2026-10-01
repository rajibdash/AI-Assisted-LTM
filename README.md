# AI-Assisted LTM: standards context and local experiment

This repository keeps the Release 20 standards study separate from a small,
executable AI/ML experiment. The experiment is not a 3GPP implementation and
does not control radio procedures.

## Standards status

The source is [3GPP TR 38.745 V20.0.0 (March 2026)](standards/references/38745-k00.pdf).
It is a study report, not an implementation specification: the cover states
that it has not been approved for implementation. Its clause 5 recommends
multi-hop UE trajectory, AI/ML-assisted intra-CU LTM and AI/ML-assisted
inter-CU LTM for the Rel-20 normative phase.

The report studies possible model placements, inputs/outputs/feedback and
potential F1/Xn impacts (clauses 4.1–4.3). It does not specify numeric HIT
improvements, elimination of TTT, direct AI control of MAC CEs or TCI, or a
“UPC Scheduler.” Existing LTM and intra-CU LTM are referenced to TS 38.300 and
TS 38.401 respectively; see the [standards handover](docs/AI_ML_LTM_Handover_Context.md)
for details and page references. The [experimental design note](docs/AI_ML_Mobility.md)
contains proposals that must not be read as normative requirements.

## Repository layout

```text
.
├── .gitignore
├── config/
│   └── default.json
├── docs/
│   ├── AI_ML_LTM_Handover_Context.md
│   └── AI_ML_Mobility.md
├── standards/
│   └── references/
│       ├── 38321-j40_MAC Spec.pdf
│       ├── 38745-k00.pdf
│       ├── AI_ML_LTM_Handover_Analysis.pdf
│       └── reducing-handover-interruption-l1l2-triggered-mobility.pdf
├── native/
│   ├── CMakeLists.txt
│   ├── include/ltm_native/
│   │   └── mobility_score.hpp
│   ├── src/
│   │   ├── bindings.cpp
│   │   └── mobility_score.cpp
│   └── tests/
│       └── test_mobility_score.cpp
├── src/ltm_agent/
│   ├── __init__.py
│   ├── __main__.py
│   ├── agent.py
│   ├── cli.py
│   ├── config.py
│   ├── data.py
│   ├── evaluation.py
│   ├── model.py
│   ├── native_scorer.py
│   └── training.py
├── tests/
│   ├── test_ltm_agent.py
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
library; setuptools is only used as the package build backend.

```sh
python -m venv .venv
. .venv/bin/activate
python -m pip install -e .
```

On Windows PowerShell, activate with `.venv\Scripts\Activate.ps1`.

Installing in editable mode also compiles the optional `ltm_native` C++
extension described below (a C++17 compiler is required; `pybind11` is
installed automatically as a build dependency).

## Test

The tests use Python's built-in `unittest` runner:

```sh
python -m unittest discover -s tests -v
```

## Python↔C++ binding (`ltm_native`)

`native/` contains a small, self-contained C++ component — a heuristic
"handover-candidate" pre-filter (`ltm_native::HandoverScorer`) — exposed to
Python as the `ltm_native` extension module via
[pybind11](https://github.com/pybind/pybind11). It uses the same two
mobility features as the rest of the experiment (`neighbor_margin_db`,
`target_load`) and the same validation rules, but is a separate, explicit
heuristic — not the trained model in `src/ltm_agent/model.py`. Like the rest
of this repository it is advisory-only.

```text
native/
├── CMakeLists.txt              # native-only build for the C++ unit test
├── include/ltm_native/
│   └── mobility_score.hpp      # public C++ interface (HandoverScorer, validation)
├── src/
│   ├── mobility_score.cpp      # scorer implementation
│   └── bindings.cpp            # thin pybind11 adapter -> `ltm_native` module
└── tests/
    └── test_mobility_score.cpp # native C++ tests (no Python required)
```

`src/ltm_agent/native_scorer.py` is the Python wrapper that imports
`ltm_native` and exposes `native_recommend(features, threshold=0.5)`; it
raises a clear `ImportError` if the extension was never built. Python-level
behavior (success and edge/error cases) is covered by
`tests/test_native_scorer.py`, which exercises the compiled extension
through that wrapper.

```sh
# Build and use from Python (also done by `pip install -e .`):
python -m pip install -e .
python -c "import ltm_native; print(ltm_native.HandoverScorer().evaluate(8.0, 0.2))"

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
