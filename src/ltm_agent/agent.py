from dataclasses import dataclass

from .config import AgentConfig
from .data import MobilityFeatures
from .model import LogisticRegression


@dataclass(frozen=True)
class Recommendation:
    candidate_recommended: bool
    probability: float


class MobilityAgent:
    def __init__(self, model: LogisticRegression, config: AgentConfig) -> None:
        self.model = model
        self.config = config

    def recommend(self, features: MobilityFeatures) -> Recommendation:
        probability = self.model.predict_probability(features)
        return Recommendation(
            candidate_recommended=probability >= self.config.decision_threshold,
            probability=probability,
        )
