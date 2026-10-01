// Native C++ tests for the `ltm_native` trajectory/recommender domain
// model (`MobilityObservation`, `TrajectoryWindow`, `LtmRecommender`,
// `FeedbackRecord`). Runnable without Python or pybind11 via CMake/CTest,
// see `native/tests/test_mobility_score.cpp` for the build commands.
#include "ltm_native/trajectory.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

int failures = 0;

void check(bool condition, const std::string &message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        ++failures;
    }
}

ltm_native::MobilityObservation make_observation(const std::string &cell_id,
                                                   double margin,
                                                   double load,
                                                   int sequence_index) {
    return ltm_native::MobilityObservation{cell_id, margin, load, sequence_index};
}

void test_validate_observation_accepts_valid_values() {
    ltm_native::validate_observation(make_observation("cell-1", 5.0, 0.4, 0));
}

void test_validate_observation_rejects_empty_cell_id() {
    bool threw = false;
    try {
        ltm_native::validate_observation(make_observation("", 5.0, 0.4, 0));
    } catch (const std::invalid_argument &) {
        threw = true;
    }
    check(threw, "validate_observation should reject an empty cell_id");
}

void test_validate_observation_rejects_negative_sequence_index() {
    bool threw = false;
    try {
        ltm_native::validate_observation(make_observation("cell-1", 5.0, 0.4, -1));
    } catch (const std::invalid_argument &) {
        threw = true;
    }
    check(threw, "validate_observation should reject a negative sequence_index");
}

void test_validate_observation_rejects_invalid_features() {
    bool threw = false;
    try {
        ltm_native::validate_observation(make_observation("cell-1", 5.0, 1.5, 0));
    } catch (const std::invalid_argument &) {
        threw = true;
    }
    check(threw, "validate_observation should reuse feature validation");
}

void test_trajectory_window_accumulates_observations_in_order() {
    ltm_native::TrajectoryWindow window;
    window.add_observation(make_observation("cell-1", -2.0, 0.6, 0));
    window.add_observation(make_observation("cell-2", 1.0, 0.5, 1));
    window.add_observation(make_observation("cell-3", 6.0, 0.3, 2));

    check(window.size() == 3, "window should contain three observations");
    check(!window.empty(), "window should not be empty after adding observations");
    check(std::fabs(window.margin_trend() - 8.0) < 1e-9, "margin_trend should be last - first margin");
    check(std::fabs(window.average_target_load() - 0.4666666667) < 1e-6,
          "average_target_load should be the arithmetic mean");
}

void test_trajectory_window_rejects_out_of_order_sequence_index() {
    ltm_native::TrajectoryWindow window;
    window.add_observation(make_observation("cell-1", 1.0, 0.5, 5));
    bool threw = false;
    try {
        window.add_observation(make_observation("cell-2", 1.0, 0.5, 2));
    } catch (const std::invalid_argument &) {
        threw = true;
    }
    check(threw, "window should reject a decreasing sequence_index");
}

void test_empty_window_aggregate_methods_throw() {
    ltm_native::TrajectoryWindow window;
    bool trend_threw = false;
    bool average_threw = false;
    try {
        window.margin_trend();
    } catch (const std::logic_error &) {
        trend_threw = true;
    }
    try {
        window.average_target_load();
    } catch (const std::logic_error &) {
        average_threw = true;
    }
    check(trend_threw, "margin_trend should throw on an empty window");
    check(average_threw, "average_target_load should throw on an empty window");
}

void test_recommend_returns_fields_and_reasons() {
    ltm_native::LtmRecommender recommender;
    const auto result = recommender.recommend(make_observation("cell-7", 10.0, 0.1, 0), 0.5);
    check(result.cell_id == "cell-7", "recommendation should carry the observation's cell_id");
    check(result.urgency > 0.0 && result.urgency < 1.0, "urgency should be a probability");
    check(result.confidence >= 0.0 && result.confidence <= 1.0, "confidence should be in [0, 1]");
    check(result.recommended, "strong margin and low load should be recommended");
    check(!result.reasons.empty(), "recommendation should include decision reasons");
    check(result.model_placement == ltm_native::ModelPlacement::GnbTrainingAndInference,
          "default placement should be GnbTrainingAndInference");
}

void test_recommend_rejects_invalid_threshold() {
    ltm_native::LtmRecommender recommender;
    bool threw = false;
    try {
        recommender.recommend(make_observation("cell-1", 1.0, 0.5, 0), 1.5);
    } catch (const std::invalid_argument &) {
        threw = true;
    }
    check(threw, "recommend should reject a threshold outside [0, 1]");
}

void test_recommend_rejects_invalid_observation() {
    ltm_native::LtmRecommender recommender;
    bool threw = false;
    try {
        recommender.recommend(make_observation("", 1.0, 0.5, 0), 0.5);
    } catch (const std::invalid_argument &) {
        threw = true;
    }
    check(threw, "recommend should reject an invalid observation");
}

void test_recommend_window_requires_non_empty_window() {
    ltm_native::LtmRecommender recommender;
    ltm_native::TrajectoryWindow window;
    bool threw = false;
    try {
        recommender.recommend_window(window, 0.5);
    } catch (const std::logic_error &) {
        threw = true;
    }
    check(threw, "recommend_window should reject an empty window");
}

void test_recommend_window_improving_trend_increases_urgency() {
    ltm_native::LtmRecommender recommender;

    ltm_native::TrajectoryWindow improving;
    improving.add_observation(make_observation("cell-1", -5.0, 0.5, 0));
    improving.add_observation(make_observation("cell-2", 1.0, 0.5, 1));

    ltm_native::TrajectoryWindow flat;
    flat.add_observation(make_observation("cell-1", 1.0, 0.5, 0));
    flat.add_observation(make_observation("cell-2", 1.0, 0.5, 1));

    const auto improving_result = recommender.recommend_window(improving, 0.5);
    const auto flat_result = recommender.recommend_window(flat, 0.5);
    check(improving_result.urgency > flat_result.urgency,
          "an improving margin trend should increase urgency relative to a flat trend");

    bool has_trend_reason = false;
    for (const auto &reason : improving_result.reasons) {
        if (reason == "margin_trend_improving") {
            has_trend_reason = true;
        }
    }
    check(has_trend_reason, "an improving trend should be reported in reasons");
}

void test_recommend_window_uses_latest_observation_cell_id() {
    ltm_native::LtmRecommender recommender;
    ltm_native::TrajectoryWindow window;
    window.add_observation(make_observation("cell-1", 1.0, 0.5, 0));
    window.add_observation(make_observation("cell-2", 2.0, 0.4, 1));
    const auto result = recommender.recommend_window(window, 0.5);
    check(result.cell_id == "cell-2", "recommend_window should report the latest hop's cell_id");
}

void test_feedback_history_accumulates_records() {
    ltm_native::LtmRecommender recommender;
    check(recommender.feedback_history().empty(), "feedback history should start empty");
    recommender.record_feedback(ltm_native::FeedbackRecord{"cell-1", true, 5.0});
    recommender.record_feedback(ltm_native::FeedbackRecord{"cell-2", false, -3.0});
    check(recommender.feedback_history().size() == 2, "feedback history should accumulate records");
    check(recommender.feedback_history()[0].cell_id == "cell-1",
          "feedback history should preserve insertion order");
}

void test_model_placement_round_trips_and_has_readable_names() {
    ltm_native::LtmRecommender oam_recommender(ltm_native::ModelPlacement::OamTrainingGnbInference);
    check(oam_recommender.model_placement() == ltm_native::ModelPlacement::OamTrainingGnbInference,
          "model_placement accessor should return the constructed placement");
    check(std::string(ltm_native::to_string(ltm_native::ModelPlacement::OamTrainingGnbInference)) ==
              "oam_training_gnb_inference",
          "to_string should return a readable name for OamTrainingGnbInference");
    check(std::string(ltm_native::to_string(ltm_native::ModelPlacement::GnbTrainingAndInference)) ==
              "gnb_training_and_inference",
          "to_string should return a readable name for GnbTrainingAndInference");
}

}  // namespace

int main() {
    test_validate_observation_accepts_valid_values();
    test_validate_observation_rejects_empty_cell_id();
    test_validate_observation_rejects_negative_sequence_index();
    test_validate_observation_rejects_invalid_features();
    test_trajectory_window_accumulates_observations_in_order();
    test_trajectory_window_rejects_out_of_order_sequence_index();
    test_empty_window_aggregate_methods_throw();
    test_recommend_returns_fields_and_reasons();
    test_recommend_rejects_invalid_threshold();
    test_recommend_rejects_invalid_observation();
    test_recommend_window_requires_non_empty_window();
    test_recommend_window_improving_trend_increases_urgency();
    test_recommend_window_uses_latest_observation_cell_id();
    test_feedback_history_accumulates_records();
    test_model_placement_round_trips_and_has_readable_names();

    if (failures == 0) {
        std::cout << "All native ltm_native trajectory tests passed.\n";
        return 0;
    }
    std::cerr << failures << " native ltm_native trajectory test(s) failed.\n";
    return 1;
}
