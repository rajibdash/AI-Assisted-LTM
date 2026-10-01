from collections.abc import Sequence

from .data import MobilityExample
from .model import LogisticRegression


def train_model(
    examples: Sequence[MobilityExample], epochs: int, learning_rate: float
) -> LogisticRegression:
    return LogisticRegression().fit(
        examples, epochs=epochs, learning_rate=learning_rate
    )
