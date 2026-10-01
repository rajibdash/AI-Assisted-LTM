"""Tests for the Python<->C++ binding (`ltm_native` + `native_scorer`).

These tests exercise the compiled pybind11 extension through the Python
wrapper in `ltm_agent.native_scorer`, covering success paths as well as
edge/error cases (invalid features, invalid threshold, and the behaviour
when the extension is unavailable).
"""

import unittest

from ltm_agent import native_scorer
from ltm_agent.data import MobilityFeatures

try:
    import ltm_native

    NATIVE_EXTENSION_BUILT = True
except ImportError:
    NATIVE_EXTENSION_BUILT = False


@unittest.skipUnless(
    NATIVE_EXTENSION_BUILT,
    "ltm_native extension is not built; run 'python -m pip install -e .'",
)
class NativeScorerTests(unittest.TestCase):
    def test_is_available_reports_true_when_built(self) -> None:
        self.assertTrue(native_scorer.is_available())

    def test_strong_neighbor_and_low_load_is_recommended(self) -> None:
        features = MobilityFeatures(neighbor_margin_db=10.0, target_load=0.1)
        result = native_scorer.native_recommend(features)
        self.assertTrue(result.candidate_recommended)
        self.assertGreaterEqual(result.urgency, 0.5)

    def test_weak_neighbor_and_high_load_is_not_recommended(self) -> None:
        features = MobilityFeatures(neighbor_margin_db=-8.0, target_load=0.9)
        result = native_scorer.native_recommend(features)
        self.assertFalse(result.candidate_recommended)
        self.assertLess(result.urgency, 0.5)

    def test_urgency_is_a_probability(self) -> None:
        features = MobilityFeatures(neighbor_margin_db=2.0, target_load=0.4)
        result = native_scorer.native_recommend(features)
        self.assertGreater(result.urgency, 0.0)
        self.assertLess(result.urgency, 1.0)

    def test_threshold_controls_the_recommendation(self) -> None:
        features = MobilityFeatures(neighbor_margin_db=1.0, target_load=0.5)
        lenient = native_scorer.native_recommend(features, threshold=0.01)
        strict = native_scorer.native_recommend(features, threshold=0.99)
        self.assertTrue(lenient.candidate_recommended)
        self.assertFalse(strict.candidate_recommended)

    def test_invalid_threshold_raises_value_error(self) -> None:
        features = MobilityFeatures(neighbor_margin_db=1.0, target_load=0.5)
        with self.assertRaises(ValueError):
            native_scorer.native_recommend(features, threshold=1.5)

    def test_invalid_features_are_rejected_before_reaching_native_code(self) -> None:
        with self.assertRaises(ValueError):
            MobilityFeatures(neighbor_margin_db=1.0, target_load=1.5).as_tuple()

    def test_custom_scorer_weights_change_the_outcome(self) -> None:
        features = MobilityFeatures(neighbor_margin_db=0.0, target_load=0.5)
        neutral_scorer = ltm_native.HandoverScorer(
            margin_weight=0.0, load_weight=0.0, bias=0.0
        )
        result = native_scorer.native_recommend(
            features, threshold=0.5, scorer=neutral_scorer
        )
        self.assertAlmostEqual(result.urgency, 0.5, places=6)

    def test_native_validate_features_matches_python_validation(self) -> None:
        with self.assertRaises(ValueError):
            ltm_native.validate_features(float("nan"), 0.5)
        with self.assertRaises(ValueError):
            ltm_native.validate_features(1.0, -0.1)
        # Does not raise for in-range values.
        ltm_native.validate_features(1.0, 0.5)


class NativeScorerUnavailableTests(unittest.TestCase):
    def test_native_recommend_raises_import_error_without_extension(self) -> None:
        original = native_scorer._ltm_native
        native_scorer._ltm_native = None
        try:
            features = MobilityFeatures(neighbor_margin_db=1.0, target_load=0.5)
            with self.assertRaises(ImportError):
                native_scorer.native_recommend(features)
            self.assertFalse(native_scorer.is_available())
        finally:
            native_scorer._ltm_native = original


if __name__ == "__main__":
    unittest.main()
