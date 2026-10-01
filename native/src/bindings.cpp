// Pybind11 bindings that expose the `ltm_native::HandoverScorer` C++ class
// to Python as the `ltm_native` extension module.
//
// Keep this file a thin adapter: validation and all business logic live in
// `mobility_score.{hpp,cpp}` so the native logic can be unit tested in C++
// independently of Python (see `native/tests/test_mobility_score.cpp`).
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <stdexcept>

#include "ltm_native/mobility_score.hpp"
#include "ltm_native/trajectory.hpp"

namespace py = pybind11;

PYBIND11_MODULE(ltm_native, module) {
    module.doc() =
        "Native heuristic handover-candidate pre-filter and trajectory/"
        "recommendation domain model for the AI-Assisted-LTM synthetic "
        "experiment. Advisory-only; does not implement or control any "
        "3GPP radio procedure.";

    py::class_<ltm_native::HandoverScore>(module, "HandoverScore")
        .def_readonly("urgency", &ltm_native::HandoverScore::urgency)
        .def_readonly("recommended", &ltm_native::HandoverScore::recommended)
        .def("__repr__", [](const ltm_native::HandoverScore &result) {
            return "HandoverScore(urgency=" + std::to_string(result.urgency) +
                   ", recommended=" + (result.recommended ? "True" : "False") + ")";
        });

    py::class_<ltm_native::HandoverScorer>(module, "HandoverScorer")
        .def(py::init<double, double, double>(),
             py::arg("margin_weight") = 0.35,
             py::arg("load_weight") = 5.0,
             py::arg("bias") = -2.0,
             "Create a scorer with explicit heuristic weights.")
        .def("score", &ltm_native::HandoverScorer::score,
             py::arg("neighbor_margin_db"), py::arg("target_load"),
             "Return the urgency probability for the given features.")
        .def("evaluate", &ltm_native::HandoverScorer::evaluate,
             py::arg("neighbor_margin_db"), py::arg("target_load"),
             py::arg("threshold") = 0.5,
             "Return a HandoverScore with the urgency and recommendation.");

    module.def(
        "validate_features",
        [](double neighbor_margin_db, double target_load) {
            ltm_native::validate_features(neighbor_margin_db, target_load);
        },
        py::arg("neighbor_margin_db"), py::arg("target_load"),
        "Validate feature ranges; raises ValueError on failure.");

    py::enum_<ltm_native::ModelPlacement>(
        module, "ModelPlacement",
        "AI/ML training/inference placement metadata (TR 38.745 clauses "
        "4.1.2.1, 4.2.2.1). Descriptive only; no training or inference "
        "actually runs in a separate process or node.")
        .value("OAM_TRAINING_GNB_INFERENCE", ltm_native::ModelPlacement::OamTrainingGnbInference)
        .value("GNB_TRAINING_AND_INFERENCE", ltm_native::ModelPlacement::GnbTrainingAndInference);

    py::class_<ltm_native::MobilityObservation>(
        module, "MobilityObservation",
        "One sample in a simplified multi-hop trajectory: cell_id, the two "
        "base mobility features, and a chronological sequence_index.")
        .def(py::init<std::string, double, double, int>(),
             py::arg("cell_id"), py::arg("neighbor_margin_db"), py::arg("target_load"),
             py::arg("sequence_index"))
        .def_readwrite("cell_id", &ltm_native::MobilityObservation::cell_id)
        .def_readwrite("neighbor_margin_db", &ltm_native::MobilityObservation::neighbor_margin_db)
        .def_readwrite("target_load", &ltm_native::MobilityObservation::target_load)
        .def_readwrite("sequence_index", &ltm_native::MobilityObservation::sequence_index)
        .def("__repr__", [](const ltm_native::MobilityObservation &observation) {
            return "MobilityObservation(cell_id='" + observation.cell_id +
                   "', neighbor_margin_db=" + std::to_string(observation.neighbor_margin_db) +
                   ", target_load=" + std::to_string(observation.target_load) +
                   ", sequence_index=" + std::to_string(observation.sequence_index) + ")";
        });

    module.def(
        "validate_observation",
        [](const ltm_native::MobilityObservation &observation) {
            ltm_native::validate_observation(observation);
        },
        py::arg("observation"),
        "Validate an observation's fields; raises ValueError on failure.");

    py::class_<ltm_native::TrajectoryWindow>(
        module, "TrajectoryWindow",
        "Ordered collection of MobilityObservation samples representing a "
        "simplified multi-hop UE trajectory (TR 38.745 clause 4.1.1).")
        .def(py::init<>())
        .def("add_observation", &ltm_native::TrajectoryWindow::add_observation,
             py::arg("observation"),
             "Validate and append an observation; raises ValueError on invalid "
             "input or a decreasing sequence_index.")
        .def("observations", &ltm_native::TrajectoryWindow::observations,
             "Return the observations currently in the window, in order.")
        .def("__len__", &ltm_native::TrajectoryWindow::size)
        .def("empty", &ltm_native::TrajectoryWindow::empty)
        .def("margin_trend", &ltm_native::TrajectoryWindow::margin_trend,
             "Last minus first neighbor_margin_db in the window.")
        .def("average_target_load", &ltm_native::TrajectoryWindow::average_target_load,
             "Arithmetic mean of target_load across the window.");

    py::class_<ltm_native::CandidateRecommendation>(
        module, "CandidateRecommendation",
        "Result of LtmRecommender.recommend/recommend_window: urgency, "
        "confidence, the recommendation flag, decision reasons, and the "
        "model-placement metadata tag.")
        .def_readonly("cell_id", &ltm_native::CandidateRecommendation::cell_id)
        .def_readonly("urgency", &ltm_native::CandidateRecommendation::urgency)
        .def_readonly("confidence", &ltm_native::CandidateRecommendation::confidence)
        .def_readonly("recommended", &ltm_native::CandidateRecommendation::recommended)
        .def_readonly("reasons", &ltm_native::CandidateRecommendation::reasons)
        .def_readonly("model_placement", &ltm_native::CandidateRecommendation::model_placement)
        .def("__repr__", [](const ltm_native::CandidateRecommendation &result) {
            return "CandidateRecommendation(cell_id='" + result.cell_id +
                   "', urgency=" + std::to_string(result.urgency) +
                   ", confidence=" + std::to_string(result.confidence) +
                   ", recommended=" + (result.recommended ? "True" : "False") + ")";
        });

    py::class_<ltm_native::FeedbackRecord>(
        module, "FeedbackRecord",
        "Local, in-memory record correlating a prior candidate cell with an "
        "observed outcome. No network or protocol behavior.")
        .def(py::init<std::string, bool, double>(),
             py::arg("cell_id"), py::arg("outcome_accepted"), py::arg("observed_margin_db"))
        .def_readwrite("cell_id", &ltm_native::FeedbackRecord::cell_id)
        .def_readwrite("outcome_accepted", &ltm_native::FeedbackRecord::outcome_accepted)
        .def_readwrite("observed_margin_db", &ltm_native::FeedbackRecord::observed_margin_db);

    py::class_<ltm_native::LtmRecommender>(
        module, "LtmRecommender",
        "Recommender combining the base HandoverScorer heuristic with the "
        "observation/window/feedback domain model.")
        .def(py::init<ltm_native::ModelPlacement, double, double, double>(),
             py::arg("placement") = ltm_native::ModelPlacement::GnbTrainingAndInference,
             py::arg("margin_weight") = 0.35,
             py::arg("load_weight") = 5.0,
             py::arg("bias") = -2.0)
        .def("recommend", &ltm_native::LtmRecommender::recommend,
             py::arg("observation"), py::arg("threshold") = 0.5,
             "Recommend based on a single MobilityObservation.")
        .def("recommend_window", &ltm_native::LtmRecommender::recommend_window,
             py::arg("window"), py::arg("threshold") = 0.5,
             "Recommend based on a TrajectoryWindow, adjusting urgency/"
             "confidence using the window's margin trend.")
        .def("record_feedback", &ltm_native::LtmRecommender::record_feedback,
             py::arg("record"), "Append a FeedbackRecord to the in-memory history.")
        .def("feedback_history", &ltm_native::LtmRecommender::feedback_history,
             "Return all FeedbackRecord entries recorded so far, in order.")
        .def("model_placement", &ltm_native::LtmRecommender::model_placement,
             "Return the ModelPlacement this recommender is tagged with.");

    py::register_exception_translator([](std::exception_ptr pointer) {
        try {
            if (pointer) {
                std::rethrow_exception(pointer);
            }
        } catch (const std::invalid_argument &error) {
            PyErr_SetString(PyExc_ValueError, error.what());
        } catch (const std::logic_error &error) {
            PyErr_SetString(PyExc_ValueError, error.what());
        }
    });
}
