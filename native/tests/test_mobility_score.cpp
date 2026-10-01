// Native C++ tests for `ltm_native::HandoverScorer`, runnable without
// Python or pybind11 via CMake/CTest:
//
//   cmake -S native -B native/build
//   cmake --build native/build
//   ctest --test-dir native/build --output-on-failure
//
// Uses a tiny hand-rolled assertion helper instead of a test framework
// dependency, consistent with the "dependency-light" style of this
// repository's Python code (see README.md).
#include "ltm_native/mobility_score.hpp"

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

void check_close(double actual, double expected, const std::string &message, double tolerance = 1e-9) {
    check(std::fabs(actual - expected) <= tolerance, message);
}

void test_validate_features_accepts_in_range_values() {
    ltm_native::validate_features(5.0, 0.5);
    ltm_native::validate_features(-8.0, 0.0);
    ltm_native::validate_features(12.0, 1.0);
}

void test_validate_features_rejects_non_finite_margin() {
    bool threw = false;
    try {
        ltm_native::validate_features(std::nan(""), 0.5);
    } catch (const std::invalid_argument &) {
        threw = true;
    }
    check(threw, "validate_features should reject NaN margin");
}

void test_validate_features_rejects_out_of_range_load() {
    bool threw_low = false;
    bool threw_high = false;
    try {
        ltm_native::validate_features(1.0, -0.1);
    } catch (const std::invalid_argument &) {
        threw_low = true;
    }
    try {
        ltm_native::validate_features(1.0, 1.1);
    } catch (const std::invalid_argument &) {
        threw_high = true;
    }
    check(threw_low, "validate_features should reject target_load below 0");
    check(threw_high, "validate_features should reject target_load above 1");
}

void test_score_is_a_probability() {
    ltm_native::HandoverScorer scorer;
    const double score = scorer.score(8.0, 0.2);
    check(score > 0.0 && score < 1.0, "score should be in the open interval (0, 1)");
}

void test_score_increases_with_margin_and_decreases_with_load() {
    ltm_native::HandoverScorer scorer;
    const double low_margin = scorer.score(-5.0, 0.5);
    const double high_margin = scorer.score(10.0, 0.5);
    check(high_margin > low_margin, "higher neighbor margin should increase urgency");

    const double low_load = scorer.score(5.0, 0.1);
    const double high_load = scorer.score(5.0, 0.9);
    check(low_load > high_load, "higher target load should decrease urgency");
}

void test_evaluate_applies_threshold() {
    ltm_native::HandoverScorer scorer;
    const auto result = scorer.evaluate(10.0, 0.1, 0.5);
    check(result.recommended == (result.urgency >= 0.5), "recommended should match the threshold comparison");

    const auto low = scorer.evaluate(-8.0, 0.9, 0.5);
    check(!low.recommended, "poor conditions should not be recommended");
}

void test_evaluate_rejects_invalid_threshold() {
    ltm_native::HandoverScorer scorer;
    bool threw = false;
    try {
        scorer.evaluate(1.0, 0.5, 1.5);
    } catch (const std::invalid_argument &) {
        threw = true;
    }
    check(threw, "evaluate should reject a threshold outside [0, 1]");
}

void test_custom_weights_change_the_score() {
    ltm_native::HandoverScorer default_scorer;
    ltm_native::HandoverScorer aggressive_scorer(/*margin_weight=*/1.0, /*load_weight=*/1.0, /*bias=*/0.0);
    const double default_score = default_scorer.score(0.0, 0.5);
    const double aggressive_score = aggressive_scorer.score(0.0, 0.5);
    check(std::fabs(default_score - aggressive_score) > 1e-6, "different weights should yield different scores");
}

void test_score_is_deterministic() {
    ltm_native::HandoverScorer scorer;
    check_close(scorer.score(3.0, 0.3), scorer.score(3.0, 0.3), "score should be deterministic for identical inputs");
}

}  // namespace

int main() {
    test_validate_features_accepts_in_range_values();
    test_validate_features_rejects_non_finite_margin();
    test_validate_features_rejects_out_of_range_load();
    test_score_is_a_probability();
    test_score_increases_with_margin_and_decreases_with_load();
    test_evaluate_applies_threshold();
    test_evaluate_rejects_invalid_threshold();
    test_custom_weights_change_the_score();
    test_score_is_deterministic();

    if (failures == 0) {
        std::cout << "All native ltm_native tests passed.\n";
        return 0;
    }
    std::cerr << failures << " native ltm_native test(s) failed.\n";
    return 1;
}
