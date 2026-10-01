#include "ltm_native/mobility_score.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace ltm_native {

void validate_features(double neighbor_margin_db, double target_load) {
    if (!std::isfinite(neighbor_margin_db)) {
        throw std::invalid_argument("neighbor_margin_db must be finite");
    }
    if (!std::isfinite(target_load) || target_load < 0.0 || target_load > 1.0) {
        throw std::invalid_argument("target_load must be between 0 and 1");
    }
}

namespace {

// Numerically stable sigmoid: clamps the logit before calling std::exp so
// extreme inputs cannot overflow, matching the clamp used by
// `ltm_agent.model.LogisticRegression.predict_probability`.
double sigmoid(double logit) {
    const double clamped = std::max(-35.0, std::min(35.0, logit));
    return 1.0 / (1.0 + std::exp(-clamped));
}

}  // namespace

HandoverScorer::HandoverScorer(double margin_weight, double load_weight, double bias)
    : margin_weight_(margin_weight), load_weight_(load_weight), bias_(bias) {}

double HandoverScorer::score(double neighbor_margin_db, double target_load) const {
    validate_features(neighbor_margin_db, target_load);
    const double logit = bias_ + margin_weight_ * neighbor_margin_db - load_weight_ * target_load;
    return sigmoid(logit);
}

HandoverScore HandoverScorer::evaluate(double neighbor_margin_db,
                                        double target_load,
                                        double threshold) const {
    if (!std::isfinite(threshold) || threshold < 0.0 || threshold > 1.0) {
        throw std::invalid_argument("threshold must be between 0 and 1");
    }
    const double urgency = score(neighbor_margin_db, target_load);
    return HandoverScore{urgency, urgency >= threshold};
}

}  // namespace ltm_native
