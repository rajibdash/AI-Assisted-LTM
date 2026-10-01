"""Ergonomic Python wrapper around the `ltm_native` trajectory/recommender
domain model: `MobilityObservation`, `TrajectoryWindow`, `LtmRecommender`,
`FeedbackRecord`, `ModelPlacement`, and `CandidateRecommendation`.

This module complements `ltm_agent.native_scorer` (the original two-feature
`HandoverScorer` pre-filter) with a richer, still-synthetic model that is
closer in shape to the concepts studied in 3GPP TR 38.745 V20.0.0:

  - a sequence of per-cell observations standing in for a simplified
    "multi-hop UE trajectory" (clause 4.1.1),
  - `ModelPlacement` metadata naming the training/inference placement
    options the report studies (clauses 4.1.2.1, 4.2.2.1) -- a descriptive
    tag only, not an active execution location,
  - a `CandidateRecommendation` with `reasons` and `confidence` for
    observability, and
  - a local, in-memory `FeedbackRecord` history.

As with the rest of this package, this is advisory-only: it does not
implement, control, or claim conformance with any 3GPP radio procedure,
message, or information element. See `docs/AI_ML_LTM_Handover_Context.md`
and `docs/AUDIT_38745_TRACEABILITY.md` for the full audit against the PDF.

If the `ltm_native` extension has not been built, constructing an
`LtmRecommender` (or calling `TrajectoryWindow._to_native`) raises a clear
`ImportError`; the plain Python dataclasses (`MobilityObservation`,
`TrajectoryWindow`, `CandidateRecommendation`, `FeedbackRecord`) remain
usable without the extension since they only validate and aggregate data.
"""

from __future__ import annotations

import enum
from dataclasses import dataclass
from typing import Iterable

from .data import MobilityFeatures

try:
    import ltm_native as _ltm_native
except ImportError:  # pragma: no cover - exercised when the extension is absent
    _ltm_native = None


def is_available() -> bool:
    """Return True when the compiled `ltm_native` extension can be used."""
    return _ltm_native is not None


def _require_native() -> None:
    if _ltm_native is None:
        raise ImportError(
            "the 'ltm_native' extension is not built; run "
            "'python -m pip install -e .' from the repository root "
            "(requires a C++17 compiler) to enable it"
        )


class ModelPlacement(enum.Enum):
    """AI/ML training/inference placement metadata.

    Names TR 38.745's studied options for multi-hop trajectory (clause
    4.1.2.1) and intra-CU LTM (clause 4.2.2.1). It is attached to a
    `CandidateRecommendation` purely as descriptive metadata; this
    repository does not actually train or run inference in OAM, a gNB, or a
    gNB-CU.
    """

    OAM_TRAINING_GNB_INFERENCE = "oam_training_gnb_inference"
    GNB_TRAINING_AND_INFERENCE = "gnb_training_and_inference"


@dataclass(frozen=True)
class MobilityObservation:
    """One sample in a simplified multi-hop trajectory.

    Extends the base `MobilityFeatures` with a `cell_id` and a
    chronological `sequence_index`, so a sequence of observations can model
    a UE moving across cells (TR 38.745 clause 4.1.1).
    """

    cell_id: str
    neighbor_margin_db: float
    target_load: float
    sequence_index: int

    def __post_init__(self) -> None:
        MobilityFeatures(
            neighbor_margin_db=self.neighbor_margin_db, target_load=self.target_load
        ).as_tuple()
        if not self.cell_id:
            raise ValueError("cell_id must not be empty")
        if self.sequence_index < 0:
            raise ValueError("sequence_index must be non-negative")


@dataclass(frozen=True)
class CandidateRecommendation:
    """Result of `LtmRecommender.recommend`/`recommend_window`."""

    cell_id: str
    urgency: float
    confidence: float
    recommended: bool
    reasons: tuple[str, ...]
    model_placement: ModelPlacement


@dataclass(frozen=True)
class FeedbackRecord:
    """Local, in-memory outcome record correlated with a prior cell_id."""

    cell_id: str
    outcome_accepted: bool
    observed_margin_db: float


class TrajectoryWindow:
    """Ordered collection of `MobilityObservation`s.

    Pure-Python aggregation (`margin_trend`, `average_target_load`) works
    without the native extension; only `LtmRecommender.recommend_window`
    requires it.
    """

    def __init__(self, observations: Iterable[MobilityObservation] = ()) -> None:
        self._observations: list[MobilityObservation] = []
        for observation in observations:
            self.add_observation(observation)

    def add_observation(self, observation: MobilityObservation) -> None:
        if (
            self._observations
            and observation.sequence_index < self._observations[-1].sequence_index
        ):
            raise ValueError(
                "sequence_index must be non-decreasing across a trajectory window"
            )
        self._observations.append(observation)

    @property
    def observations(self) -> tuple[MobilityObservation, ...]:
        return tuple(self._observations)

    def __len__(self) -> int:
        return len(self._observations)

    def margin_trend(self) -> float:
        if not self._observations:
            raise ValueError("margin_trend requires at least one observation")
        return (
            self._observations[-1].neighbor_margin_db
            - self._observations[0].neighbor_margin_db
        )

    def average_target_load(self) -> float:
        if not self._observations:
            raise ValueError("average_target_load requires at least one observation")
        return sum(o.target_load for o in self._observations) / len(
            self._observations
        )

    def _to_native(self) -> "_ltm_native.TrajectoryWindow":
        _require_native()
        window = _ltm_native.TrajectoryWindow()
        for observation in self._observations:
            window.add_observation(_to_native_observation(observation))
        return window


def _to_native_observation(
    observation: MobilityObservation,
) -> "_ltm_native.MobilityObservation":
    _require_native()
    return _ltm_native.MobilityObservation(
        observation.cell_id,
        observation.neighbor_margin_db,
        observation.target_load,
        observation.sequence_index,
    )


def _to_native_placement(placement: ModelPlacement) -> "_ltm_native.ModelPlacement":
    _require_native()
    return getattr(_ltm_native.ModelPlacement, placement.name)


def _from_native_placement(value: "_ltm_native.ModelPlacement") -> ModelPlacement:
    return ModelPlacement[value.name]


def _from_native_recommendation(
    result: "_ltm_native.CandidateRecommendation",
) -> CandidateRecommendation:
    return CandidateRecommendation(
        cell_id=result.cell_id,
        urgency=result.urgency,
        confidence=result.confidence,
        recommended=result.recommended,
        reasons=tuple(result.reasons),
        model_placement=_from_native_placement(result.model_placement),
    )


class LtmRecommender:
    """Python wrapper around the native `ltm_native.LtmRecommender`.

    Raises `ImportError` on construction if the `ltm_native` extension has
    not been built (see `is_available`).
    """

    def __init__(
        self,
        placement: ModelPlacement = ModelPlacement.GNB_TRAINING_AND_INFERENCE,
        margin_weight: float = 0.35,
        load_weight: float = 5.0,
        bias: float = -2.0,
    ) -> None:
        _require_native()
        self._placement = placement
        self._native = _ltm_native.LtmRecommender(
            _to_native_placement(placement), margin_weight, load_weight, bias
        )

    def recommend(
        self, observation: MobilityObservation, threshold: float = 0.5
    ) -> CandidateRecommendation:
        """Recommend based on a single `MobilityObservation`."""
        result = self._native.recommend(
            _to_native_observation(observation), threshold
        )
        return _from_native_recommendation(result)

    def recommend_window(
        self, window: TrajectoryWindow, threshold: float = 0.5
    ) -> CandidateRecommendation:
        """Recommend based on a `TrajectoryWindow`.

        Adjusts the latest observation's urgency/confidence using the
        window's margin trend (see `native/src/trajectory.cpp`). Raises
        `ValueError` for an empty window.
        """
        if len(window) == 0:
            raise ValueError("recommend_window requires a non-empty window")
        result = self._native.recommend_window(window._to_native(), threshold)
        return _from_native_recommendation(result)

    def record_feedback(self, record: FeedbackRecord) -> None:
        """Append `record` to the in-memory feedback history."""
        self._native.record_feedback(
            _ltm_native.FeedbackRecord(
                record.cell_id, record.outcome_accepted, record.observed_margin_db
            )
        )

    def feedback_history(self) -> tuple[FeedbackRecord, ...]:
        """Return all recorded `FeedbackRecord`s, in insertion order."""
        return tuple(
            FeedbackRecord(
                cell_id=entry.cell_id,
                outcome_accepted=entry.outcome_accepted,
                observed_margin_db=entry.observed_margin_db,
            )
            for entry in self._native.feedback_history()
        )

    @property
    def model_placement(self) -> ModelPlacement:
        return self._placement
