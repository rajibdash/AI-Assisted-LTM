#include "ltm_native/trajectory.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>

namespace ltm_native {

const char *to_string(ModelPlacement placement) {
    switch (placement) {
        case ModelPlacement::OamTrainingGnbInference:
            return "oam_training_gnb_inference";
        case ModelPlacement::GnbTrainingAndInference:
            return "gnb_training_and_inference";
    }
    return "unknown_model_placement";
}

void validate_observation(const MobilityObservation &observation) {
    validate_features(observation.neighbor_margin_db, observation.target_load);
    if (observation.cell_id.empty()) {
        throw std::invalid_argument("cell_id must not be empty");
    }
    if (observation.sequence_index < 0) {
        throw std::invalid_argument("sequence_index must be non-negative");
    }
}

void TrajectoryWindow::add_observation(MobilityObservation observation) {
    validate_observation(observation);
    if (!observations_.empty() && observation.sequence_index < observations_.back().sequence_index) {
        throw std::invalid_argument(
            "sequence_index must be non-decreasing across a trajectory window");
    }
    observations_.push_back(std::move(observation));
}

double TrajectoryWindow::margin_trend() const {
    if (observations_.empty()) {
        throw std::logic_error("margin_trend requires at least one observation");
    }
    return observations_.back().neighbor_margin_db - observations_.front().neighbor_margin_db;
}

double TrajectoryWindow::average_target_load() const {
    if (observations_.empty()) {
        throw std::logic_error("average_target_load requires at least one observation");
    }
    const double total = std::accumulate(
        observations_.begin(), observations_.end(), 0.0,
        [](double accumulator, const MobilityObservation &observation) {
            return accumulator + observation.target_load;
        });
    return total / static_cast<double>(observations_.size());
}

namespace {

double clamp01(double value) {
    return std::max(0.0, std::min(1.0, value));
}

}  // namespace

LtmRecommender::LtmRecommender(ModelPlacement placement,
                                double margin_weight,
                                double load_weight,
                                double bias)
    : scorer_(margin_weight, load_weight, bias), placement_(placement) {}

CandidateRecommendation LtmRecommender::build_recommendation(const std::string &cell_id,
                                                               double urgency,
                                                               double threshold,
                                                               double trend_adjustment,
                                                               double neighbor_margin_db,
                                                               double target_load) const {
    if (!std::isfinite(threshold) || threshold < 0.0 || threshold > 1.0) {
        throw std::invalid_argument("threshold must be between 0 and 1");
    }

    const double adjusted_urgency = clamp01(urgency + trend_adjustment);
    const double confidence = clamp01(2.0 * std::fabs(adjusted_urgency - 0.5));
    const bool recommended = adjusted_urgency >= threshold;

    std::vector<std::string> reasons;
    if (neighbor_margin_db >= 0.0) {
        reasons.push_back("neighbor_margin_db_non_negative");
    } else {
        reasons.push_back("neighbor_margin_db_negative");
    }
    if (target_load <= 0.3) {
        reasons.push_back("target_load_low");
    } else if (target_load >= 0.7) {
        reasons.push_back("target_load_high");
    }
    if (trend_adjustment > 1e-9) {
        reasons.push_back("margin_trend_improving");
    } else if (trend_adjustment < -1e-9) {
        reasons.push_back("margin_trend_degrading");
    }
    reasons.push_back(recommended ? "urgency_at_or_above_threshold" : "urgency_below_threshold");

    return CandidateRecommendation{
        cell_id, adjusted_urgency, confidence, recommended, std::move(reasons), placement_};
}

CandidateRecommendation LtmRecommender::recommend(const MobilityObservation &observation,
                                                    double threshold) const {
    validate_observation(observation);
    const double urgency = scorer_.score(observation.neighbor_margin_db, observation.target_load);
    return build_recommendation(observation.cell_id, urgency, threshold, /*trend_adjustment=*/0.0,
                                 observation.neighbor_margin_db, observation.target_load);
}

CandidateRecommendation LtmRecommender::recommend_window(const TrajectoryWindow &window,
                                                           double threshold) const {
    if (window.empty()) {
        throw std::logic_error("recommend_window requires a non-empty window");
    }

    const MobilityObservation &latest = window.observations().back();
    const double urgency = scorer_.score(latest.neighbor_margin_db, latest.target_load);

    // A small, bounded nudge so a multi-hop improving/degrading trend shifts
    // the single-hop urgency without overwhelming it. This weighting is an
    // experimental project choice, not a 3GPP-derived formula.
    const double trend = window.margin_trend();
    const double trend_adjustment = std::max(-0.1, std::min(0.1, trend * 0.01));

    return build_recommendation(latest.cell_id, urgency, threshold, trend_adjustment,
                                 latest.neighbor_margin_db, latest.target_load);
}

void LtmRecommender::record_feedback(FeedbackRecord record) {
    feedback_.push_back(std::move(record));
}

}  // namespace ltm_native
