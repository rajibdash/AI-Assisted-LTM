from collections.abc import Sequence
from dataclasses import dataclass

from .data import MobilityExample
from .model import LogisticRegression


@dataclass(frozen=True)
class Evaluation:
    accuracy: float
    sample_count: int


def evaluate(
    model: LogisticRegression,
    examples: Sequence[MobilityExample],
    threshold: float = 0.5,
) -> Evaluation:
    if not examples:
        raise ValueError("evaluation examples must not be empty")
    if not 0 <= threshold <= 1:
        raise ValueError("threshold must be between 0 and 1")

    correct = sum(
        (model.predict_probability(example.features) >= threshold)
        == bool(example.switch_candidate)
        for example in examples
    )
    return Evaluation(accuracy=correct / len(examples), sample_count=len(examples))
