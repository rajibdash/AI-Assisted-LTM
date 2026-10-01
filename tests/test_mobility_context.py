"""Tests for the trajectory/recommender domain model
(`ltm_agent.mobility_context`), covering the pure-Python dataclasses as
well as the compiled `ltm_native` extension through `LtmRecommender`.

Covers success paths, boundary conditions, malformed input, feedback
recording, and behavior when the `ltm_native` extension is unavailable.
"""

import unittest

from ltm_agent import mobility_context as mc

try:
    import ltm_native  # noqa: F401

    NATIVE_EXTENSION_BUILT = True
except ImportError:
    NATIVE_EXTENSION_BUILT = False


class MobilityObservationTests(unittest.TestCase):
    def test_valid_observation_is_accepted(self) -> None:
        observation = mc.MobilityObservation("cell-1", 5.0, 0.4, 0)
        self.assertEqual(observation.cell_id, "cell-1")

    def test_empty_cell_id_is_rejected(self) -> None:
        with self.assertRaises(ValueError):
            mc.MobilityObservation("", 5.0, 0.4, 0)

    def test_negative_sequence_index_is_rejected(self) -> None:
        with self.assertRaises(ValueError):
            mc.MobilityObservation("cell-1", 5.0, 0.4, -1)

    def test_invalid_features_are_rejected(self) -> None:
        with self.assertRaises(ValueError):
            mc.MobilityObservation("cell-1", 5.0, 1.5, 0)
        with self.assertRaises(ValueError):
            mc.MobilityObservation("cell-1", float("nan"), 0.4, 0)


class TrajectoryWindowTests(unittest.TestCase):
    def test_window_accumulates_in_order(self) -> None:
        window = mc.TrajectoryWindow(
            [
                mc.MobilityObservation("cell-1", -2.0, 0.6, 0),
                mc.MobilityObservation("cell-2", 1.0, 0.5, 1),
                mc.MobilityObservation("cell-3", 6.0, 0.3, 2),
            ]
        )
        self.assertEqual(len(window), 3)
        self.assertAlmostEqual(window.margin_trend(), 8.0)
        self.assertAlmostEqual(window.average_target_load(), 0.4666666667, places=6)

    def test_out_of_order_sequence_index_is_rejected(self) -> None:
        window = mc.TrajectoryWindow()
        window.add_observation(mc.MobilityObservation("cell-1", 1.0, 0.5, 5))
        with self.assertRaises(ValueError):
            window.add_observation(mc.MobilityObservation("cell-2", 1.0, 0.5, 2))

    def test_empty_window_aggregates_raise(self) -> None:
        window = mc.TrajectoryWindow()
        self.assertEqual(len(window), 0)
        with self.assertRaises(ValueError):
            window.margin_trend()
        with self.assertRaises(ValueError):
            window.average_target_load()


@unittest.skipUnless(
    NATIVE_EXTENSION_BUILT,
    "ltm_native extension is not built; run 'python -m pip install -e .'",
)
class LtmRecommenderTests(unittest.TestCase):
    def test_is_available_reports_true_when_built(self) -> None:
        self.assertTrue(mc.is_available())

    def test_recommend_returns_structured_result(self) -> None:
        recommender = mc.LtmRecommender()
        observation = mc.MobilityObservation("cell-7", 10.0, 0.1, 0)
        result = recommender.recommend(observation)
        self.assertIsInstance(result, mc.CandidateRecommendation)
        self.assertEqual(result.cell_id, "cell-7")
        self.assertTrue(result.recommended)
        self.assertGreater(result.urgency, 0.0)
        self.assertLess(result.urgency, 1.0)
        self.assertGreaterEqual(result.confidence, 0.0)
        self.assertLessEqual(result.confidence, 1.0)
        self.assertIn("urgency_at_or_above_threshold", result.reasons)
        self.assertEqual(
            result.model_placement, mc.ModelPlacement.GNB_TRAINING_AND_INFERENCE
        )

    def test_recommend_rejects_invalid_threshold(self) -> None:
        recommender = mc.LtmRecommender()
        observation = mc.MobilityObservation("cell-1", 1.0, 0.5, 0)
        with self.assertRaises(ValueError):
            recommender.recommend(observation, threshold=1.5)

    def test_recommend_window_requires_non_empty_window(self) -> None:
        recommender = mc.LtmRecommender()
        with self.assertRaises(ValueError):
            recommender.recommend_window(mc.TrajectoryWindow())

    def test_recommend_window_improving_trend_increases_urgency(self) -> None:
        recommender = mc.LtmRecommender()
        improving = mc.TrajectoryWindow(
            [
                mc.MobilityObservation("cell-1", -5.0, 0.5, 0),
                mc.MobilityObservation("cell-2", 1.0, 0.5, 1),
            ]
        )
        flat = mc.TrajectoryWindow(
            [
                mc.MobilityObservation("cell-1", 1.0, 0.5, 0),
                mc.MobilityObservation("cell-2", 1.0, 0.5, 1),
            ]
        )
        improving_result = recommender.recommend_window(improving)
        flat_result = recommender.recommend_window(flat)
        self.assertGreater(improving_result.urgency, flat_result.urgency)
        self.assertIn("margin_trend_improving", improving_result.reasons)
        self.assertEqual(improving_result.cell_id, "cell-2")

    def test_feedback_history_round_trips(self) -> None:
        recommender = mc.LtmRecommender()
        self.assertEqual(recommender.feedback_history(), ())
        recommender.record_feedback(mc.FeedbackRecord("cell-1", True, 5.0))
        recommender.record_feedback(mc.FeedbackRecord("cell-2", False, -3.0))
        history = recommender.feedback_history()
        self.assertEqual(len(history), 2)
        self.assertEqual(history[0], mc.FeedbackRecord("cell-1", True, 5.0))
        self.assertEqual(history[1].cell_id, "cell-2")

    def test_model_placement_is_configurable(self) -> None:
        recommender = mc.LtmRecommender(
            placement=mc.ModelPlacement.OAM_TRAINING_GNB_INFERENCE
        )
        self.assertEqual(
            recommender.model_placement, mc.ModelPlacement.OAM_TRAINING_GNB_INFERENCE
        )
        observation = mc.MobilityObservation("cell-1", 10.0, 0.1, 0)
        result = recommender.recommend(observation)
        self.assertEqual(
            result.model_placement, mc.ModelPlacement.OAM_TRAINING_GNB_INFERENCE
        )


class LtmRecommenderUnavailableTests(unittest.TestCase):
    def test_constructor_raises_import_error_without_extension(self) -> None:
        original = mc._ltm_native
        mc._ltm_native = None
        try:
            self.assertFalse(mc.is_available())
            with self.assertRaises(ImportError):
                mc.LtmRecommender()
        finally:
            mc._ltm_native = original

    def test_trajectory_window_aggregation_works_without_extension(self) -> None:
        original = mc._ltm_native
        mc._ltm_native = None
        try:
            window = mc.TrajectoryWindow(
                [
                    mc.MobilityObservation("cell-1", -2.0, 0.6, 0),
                    mc.MobilityObservation("cell-2", 6.0, 0.3, 1),
                ]
            )
            self.assertAlmostEqual(window.margin_trend(), 8.0)
            with self.assertRaises(ImportError):
                window._to_native()
        finally:
            mc._ltm_native = original


if __name__ == "__main__":
    unittest.main()
