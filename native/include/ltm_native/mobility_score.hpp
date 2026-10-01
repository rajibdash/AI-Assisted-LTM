// Small, dependency-free C++ component used to demonstrate Python<->C++
// bindings for this repository.
//
// Scope: this is a *heuristic* native pre-filter for the handover-candidate
// decision, not a replacement for the trained Python model in
// `src/ltm_agent/model.py`. It mirrors the same two input features
// (`neighbor_margin_db`, `target_load`) and the same validation rules as
// `ltm_agent.data.MobilityFeatures.as_tuple`, so it stays consistent with
// the rest of the synthetic experiment. Like the rest of this repository,
// it is advisory-only and does not implement or control any 3GPP radio
// procedure.
#pragma once

namespace ltm_native {

// Validates the two mobility features used throughout this repository.
//
// Mirrors `ltm_agent.data.MobilityFeatures.as_tuple`:
//   - `neighbor_margin_db` must be a finite number.
//   - `target_load` must be finite and within the closed interval [0, 1].
//
// Throws `std::invalid_argument` with a descriptive message when a rule is
// violated.
void validate_features(double neighbor_margin_db, double target_load);

// Result of evaluating the handover heuristic for a single sample.
struct HandoverScore {
    // Urgency/likelihood that the neighbor cell is a good handover
    // candidate, expressed as a probability in (0, 1).
    double urgency;
    // True when `urgency` is at or above the configured decision threshold.
    bool recommended;
};

// A minimal, explicit heuristic scorer.
//
// `urgency = sigmoid(bias + margin_weight * neighbor_margin_db
//                          - load_weight * target_load)`
//
// Intuitively: a larger neighbor margin (stronger neighbor signal) and a
// lower target cell load both increase the urgency to recommend switching,
// matching the synthetic label rule used to generate training data in
// `ltm_agent.data.synthetic_examples`.
class HandoverScorer {
public:
    explicit HandoverScorer(double margin_weight = 0.35,
                             double load_weight = 5.0,
                             double bias = -2.0);

    // Returns the urgency probability for the given features.
    // Throws `std::invalid_argument` for invalid inputs (see
    // `validate_features`).
    double score(double neighbor_margin_db, double target_load) const;

    // Returns both the urgency score and the thresholded recommendation.
    // `threshold` must be within the closed interval [0, 1].
    HandoverScore evaluate(double neighbor_margin_db,
                            double target_load,
                            double threshold = 0.5) const;

private:
    double margin_weight_;
    double load_weight_;
    double bias_;
};

}  // namespace ltm_native
