// Small, dependency-free C++ domain model that extends the two-feature
// heuristic in `mobility_score.hpp` with a few additional concepts drawn
// from 3GPP TR 38.745 V20.0.0 ("Study on AI/ML for NG-RAN Phase 3"):
//
//   - `MobilityObservation` / `TrajectoryWindow`: a simplified stand-in for
//     the report's "multi-hop UE trajectory" concept (clause 4.1.1), i.e. a
//     chronologically ordered sequence of per-cell samples instead of a
//     single first-hop measurement.
//   - `ModelPlacement`: an enum naming the AI/ML training/inference location
//     options the report studies for multi-hop trajectory and intra-CU LTM
//     (clauses 4.1.2.1 and 4.2.2.1). It is *metadata only*: no training or
//     inference actually happens in a different process or node, and no F1/
//     Xn/OAM interface is implemented.
//   - `CandidateRecommendation` / `FeedbackRecord` / `LtmRecommender`: a
//     local, in-memory model of the report's candidate
//     input/output/feedback loop (clauses 4.1.2.2-4.1.2.4, 4.2.2.2-4.2.2.4)
//     with decision `reasons` and a `confidence` value for observability.
//
// As with the rest of this repository, everything here is an experimental,
// advisory-only synthetic model. It does not implement, control, or claim
// conformance with any 3GPP radio procedure, message, or information
// element; thresholds, weights, reason strings and the confidence formula
// are project choices, not normative values.
#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "ltm_native/mobility_score.hpp"

namespace ltm_native {

// Named AI/ML training/inference placement options. Mirrors the
// enumerations the report studies in clauses 4.1.2.1 (multi-hop trajectory)
// and 4.2.2.1 (intra-CU LTM); used here purely as descriptive metadata
// attached to a recommendation, not as an active execution location.
enum class ModelPlacement {
    OamTrainingGnbInference,       // OAM trains; gNB (or gNB-CU) infers.
    GnbTrainingAndInference,       // Training and inference co-located.
};

const char *to_string(ModelPlacement placement);

// One sample in a simplified multi-hop trajectory: the existing two
// mobility features plus a cell identifier and a monotonically increasing
// hop index, so a sequence of observations can represent a UE moving
// across cells (TR 38.745 clause 4.1.1), independent of any gNB/CU.
struct MobilityObservation {
    std::string cell_id;
    double neighbor_margin_db;
    double target_load;
    int sequence_index;
};

// Validates an observation's numeric fields (see `validate_features`) and
// requires a non-empty `cell_id` and a non-negative `sequence_index`.
// Throws `std::invalid_argument` on failure.
void validate_observation(const MobilityObservation &observation);

// An ordered collection of observations representing one windowed,
// multi-hop trajectory. Observations must be added in non-decreasing
// `sequence_index` order to keep the window chronological.
class TrajectoryWindow {
public:
    // Validates `observation` and appends it; throws `std::invalid_argument`
    // if it is invalid or its `sequence_index` is lower than the last
    // observation already in the window.
    void add_observation(MobilityObservation observation);

    const std::vector<MobilityObservation> &observations() const { return observations_; }
    std::size_t size() const { return observations_.size(); }
    bool empty() const { return observations_.empty(); }

    // Difference between the last and first `neighbor_margin_db` in the
    // window; positive means the neighbor signal has been improving across
    // hops. Throws `std::logic_error` on an empty window.
    double margin_trend() const;

    // Arithmetic mean of `target_load` across all observations in the
    // window. Throws `std::logic_error` on an empty window.
    double average_target_load() const;

private:
    std::vector<MobilityObservation> observations_;
};

// Result of evaluating a single observation or a trajectory window,
// combining the base heuristic urgency with `confidence`, human-readable
// decision `reasons`, and the `model_placement` metadata the recommendation
// is tagged with.
struct CandidateRecommendation {
    std::string cell_id;
    double urgency;
    double confidence;
    bool recommended;
    std::vector<std::string> reasons;
    ModelPlacement model_placement;
};

// A local, in-memory record correlating a prior candidate cell with an
// observed outcome. Mirrors the report's "feedback" concept (e.g. selected
// target cell, measured trajectory) as plain data only; it does not report
// anything over a network or protocol interface.
struct FeedbackRecord {
    std::string cell_id;
    bool outcome_accepted;
    double observed_margin_db;
};

// Combines the base `HandoverScorer` heuristic with the richer observation/
// window/feedback domain model described above.
class LtmRecommender {
public:
    explicit LtmRecommender(ModelPlacement placement = ModelPlacement::GnbTrainingAndInference,
                             double margin_weight = 0.35,
                             double load_weight = 5.0,
                             double bias = -2.0);

    // Recommendation for a single observation (one hop / one sample).
    // Throws `std::invalid_argument` for invalid inputs.
    CandidateRecommendation recommend(const MobilityObservation &observation,
                                       double threshold = 0.5) const;

    // Recommendation for a trajectory window: scores the latest
    // observation, then adjusts `urgency` and `confidence` using the
    // window's margin trend, and the cell_id used is that of the latest
    // observation in the window. Throws `std::logic_error` for an empty
    // window and `std::invalid_argument` for an invalid `threshold`.
    CandidateRecommendation recommend_window(const TrajectoryWindow &window,
                                              double threshold = 0.5) const;

    // Appends a feedback record to the in-memory history. Does not
    // validate against prior recommendations; callers are expected to
    // supply a `cell_id` that was previously recommended.
    void record_feedback(FeedbackRecord record);

    const std::vector<FeedbackRecord> &feedback_history() const { return feedback_; }

    ModelPlacement model_placement() const { return placement_; }

private:
    CandidateRecommendation build_recommendation(const std::string &cell_id,
                                                  double urgency,
                                                  double threshold,
                                                  double trend_adjustment,
                                                  double neighbor_margin_db,
                                                  double target_load) const;

    HandoverScorer scorer_;
    ModelPlacement placement_;
    std::vector<FeedbackRecord> feedback_;
};

}  // namespace ltm_native
