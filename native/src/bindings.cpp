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

namespace py = pybind11;

PYBIND11_MODULE(ltm_native, module) {
    module.doc() =
        "Native heuristic handover-candidate pre-filter for the "
        "AI-Assisted-LTM synthetic experiment. Advisory-only; does not "
        "implement or control any 3GPP radio procedure.";

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

    py::register_exception_translator([](std::exception_ptr pointer) {
        try {
            if (pointer) {
                std::rethrow_exception(pointer);
            }
        } catch (const std::invalid_argument &error) {
            PyErr_SetString(PyExc_ValueError, error.what());
        }
    });
}
