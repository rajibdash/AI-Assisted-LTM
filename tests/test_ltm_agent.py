import json
import tempfile
import unittest
from pathlib import Path

from ltm_agent.agent import MobilityAgent
from ltm_agent.config import AgentConfig, load_config
from ltm_agent.data import (
    MobilityFeatures,
    synthetic_examples,
    train_test_split,
)
from ltm_agent.evaluation import evaluate
from ltm_agent.model import LogisticRegression


class LtmAgentTests(unittest.TestCase):
    def test_synthetic_data_is_deterministic(self) -> None:
        self.assertEqual(synthetic_examples(20, seed=4), synthetic_examples(20, seed=4))
        with self.assertRaises(ValueError):
            MobilityFeatures(neighbor_margin_db=1, target_load=float("nan")).as_tuple()

    def test_split_is_reproducible_and_keeps_both_sets_nonempty(self) -> None:
        examples = synthetic_examples(10)
        first = train_test_split(examples, train_fraction=0.8, seed=3)
        second = train_test_split(examples, train_fraction=0.8, seed=3)
        self.assertEqual(first, second)
        self.assertEqual((len(first[0]), len(first[1])), (8, 2))

    def test_model_trains_and_evaluates_on_held_out_synthetic_data(self) -> None:
        examples = synthetic_examples(160, seed=7)
        training, test = train_test_split(examples, train_fraction=0.8, seed=7)
        model = LogisticRegression().fit(training, epochs=800, learning_rate=0.1)
        metrics = evaluate(model, test)
        self.assertEqual(metrics.sample_count, len(test))
        self.assertGreaterEqual(metrics.accuracy, 0.9)

    def test_recommendation_obeys_configured_threshold(self) -> None:
        class FixedModel:
            def predict_probability(self, features: MobilityFeatures) -> float:
                return 0.6

        features = MobilityFeatures(neighbor_margin_db=2, target_load=0.5)
        low_threshold = MobilityAgent(FixedModel(), AgentConfig(decision_threshold=0.6))
        high_threshold = MobilityAgent(FixedModel(), AgentConfig(decision_threshold=0.7))
        self.assertTrue(low_threshold.recommend(features).candidate_recommended)
        self.assertFalse(high_threshold.recommend(features).candidate_recommended)

    def test_config_loads_json_and_rejects_invalid_values(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "config.json"
            path.write_text(json.dumps({"seed": 11, "samples": 20}), encoding="utf-8")
            self.assertEqual(load_config(path).seed, 11)
            self.assertEqual(load_config(path).samples, 20)

        with self.assertRaises(ValueError):
            AgentConfig(train_fraction=1.0)


if __name__ == "__main__":
    unittest.main()
