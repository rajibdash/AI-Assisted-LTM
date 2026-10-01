import json
import math
from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class AgentConfig:
    seed: int = 7
    samples: int = 160
    train_fraction: float = 0.8
    epochs: int = 800
    learning_rate: float = 0.1
    decision_threshold: float = 0.5

    def __post_init__(self) -> None:
        if self.samples < 4:
            raise ValueError("samples must be at least 4")
        if not 0 < self.train_fraction < 1:
            raise ValueError("train_fraction must be between 0 and 1")
        if self.epochs < 1:
            raise ValueError("epochs must be positive")
        if not math.isfinite(self.learning_rate) or self.learning_rate <= 0:
            raise ValueError("learning_rate must be a finite positive number")
        if not 0 <= self.decision_threshold <= 1:
            raise ValueError("decision_threshold must be between 0 and 1")


def load_config(path: str | Path) -> AgentConfig:
    values = json.loads(Path(path).read_text(encoding="utf-8"))
    if not isinstance(values, dict):
        raise ValueError("configuration must be a JSON object")
    return AgentConfig(**values)
