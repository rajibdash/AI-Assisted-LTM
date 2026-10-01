import math
import random
from dataclasses import dataclass
from typing import Sequence


@dataclass(frozen=True)
class MobilityFeatures:
    neighbor_margin_db: float
    target_load: float

    def as_tuple(self) -> tuple[float, float]:
        if not math.isfinite(self.neighbor_margin_db):
            raise ValueError("neighbor_margin_db must be finite")
        if not math.isfinite(self.target_load) or not 0 <= self.target_load <= 1:
            raise ValueError("target_load must be between 0 and 1")
        return self.neighbor_margin_db, self.target_load


@dataclass(frozen=True)
class MobilityExample:
    features: MobilityFeatures
    switch_candidate: int

    def __post_init__(self) -> None:
        if self.switch_candidate not in (0, 1):
            raise ValueError("switch_candidate must be 0 or 1")
        self.features.as_tuple()


def synthetic_examples(count: int, seed: int = 7) -> list[MobilityExample]:
    if count < 1:
        raise ValueError("count must be positive")

    generator = random.Random(seed)
    examples = []
    for _ in range(count):
        margin = generator.uniform(-8, 12)
        load = generator.uniform(0, 1)
        candidate = int(margin > 2 + 5 * load)
        examples.append(
            MobilityExample(
                MobilityFeatures(neighbor_margin_db=margin, target_load=load),
                candidate,
            )
        )
    return examples


def train_test_split(
    examples: Sequence[MobilityExample], train_fraction: float, seed: int
) -> tuple[list[MobilityExample], list[MobilityExample]]:
    if len(examples) < 2:
        raise ValueError("at least two examples are required")
    if not 0 < train_fraction < 1:
        raise ValueError("train_fraction must be between 0 and 1")

    shuffled = list(examples)
    random.Random(seed).shuffle(shuffled)
    split_at = round(len(shuffled) * train_fraction)
    split_at = min(max(split_at, 1), len(shuffled) - 1)
    return shuffled[:split_at], shuffled[split_at:]
