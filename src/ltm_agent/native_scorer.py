"""Thin Python wrapper around the optional `ltm_native` C++ extension.

`ltm_native` (built from `native/`, see `setup.py`) implements a fast
heuristic handover-candidate pre-filter using the same two mobility
features as the rest of this package (`neighbor_margin_db`, `target_load`).
It is advisory-only and does not replace the trained
`ltm_agent.model.LogisticRegression`; it exists to demonstrate a realistic
Python<->C++ binding and to offer a cheap native pre-check before running
the full model.

If the extension has not been built (for example, a plain
`pip install .` without a C++ toolchain was never run), importing this
module still succeeds; calling `native_recommend` raises a clear
`ImportError` explaining how to build it.
"""

from __future__ import annotations

from dataclasses import dataclass

from .data import MobilityFeatures

try:
    import ltm_native as _ltm_native
except ImportError:  # pragma: no cover - exercised when the extension is absent
    _ltm_native = None


@dataclass(frozen=True)
class NativeRecommendation:
    """Result of running the native heuristic pre-filter."""

    urgency: float
    candidate_recommended: bool


def is_available() -> bool:
    """Return True when the compiled `ltm_native` extension can be used."""
    return _ltm_native is not None


def native_recommend(
    features: MobilityFeatures,
    threshold: float = 0.5,
    scorer: "_ltm_native.HandoverScorer | None" = None,
) -> NativeRecommendation:
    """Evaluate `features` using the native C++ heuristic scorer.

    Raises `ImportError` if the `ltm_native` extension has not been built
    (run `python -m pip install -e .` from the repository root, which
    requires a C++17 compiler). Raises `ValueError` for invalid features or
    threshold, mirroring the validation used elsewhere in this package.
    """
    if _ltm_native is None:
        raise ImportError(
            "the 'ltm_native' extension is not built; run "
            "'python -m pip install -e .' from the repository root "
            "(requires a C++17 compiler) to enable it"
        )

    # Reuse the shared validation rules before crossing into C++ so error
    # messages stay consistent regardless of which layer rejects first.
    neighbor_margin_db, target_load = features.as_tuple()

    active_scorer = scorer if scorer is not None else _ltm_native.HandoverScorer()
    result = active_scorer.evaluate(neighbor_margin_db, target_load, threshold)
    return NativeRecommendation(
        urgency=result.urgency, candidate_recommended=result.recommended
    )
