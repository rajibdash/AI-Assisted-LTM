import math
from collections.abc import Sequence

from .data import MobilityExample, MobilityFeatures


class LogisticRegression:
    def __init__(self) -> None:
        self.weights = [0.0, 0.0]
        self.bias = 0.0

    def fit(
        self,
        examples: Sequence[MobilityExample],
        epochs: int = 800,
        learning_rate: float = 0.1,
    ) -> "LogisticRegression":
        if not examples:
            raise ValueError("training examples must not be empty")
        if epochs < 1:
            raise ValueError("epochs must be positive")
        if not math.isfinite(learning_rate) or learning_rate <= 0:
            raise ValueError("learning_rate must be a finite positive number")
        if {example.switch_candidate for example in examples} != {0, 1}:
            raise ValueError("training data must contain both classes")

        self.weights = [0.0, 0.0]
        self.bias = 0.0
        for _ in range(epochs):
            gradients = [0.0, 0.0]
            bias_gradient = 0.0
            for example in examples:
                features = example.features.as_tuple()
                error = self.predict_probability(example.features) - example.switch_candidate
                for index, value in enumerate(features):
                    gradients[index] += error * value
                bias_gradient += error

            scale = learning_rate / len(examples)
            self.weights = [
                weight - scale * gradient
                for weight, gradient in zip(self.weights, gradients)
            ]
            self.bias -= scale * bias_gradient
        return self

    def predict_probability(self, features: MobilityFeatures) -> float:
        values = features.as_tuple()
        score = self.bias + sum(
            weight * value for weight, value in zip(self.weights, values)
        )
        score = max(min(score, 35), -35)
        return 1 / (1 + math.exp(-score))
