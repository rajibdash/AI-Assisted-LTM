cmake_minimum_required(VERSION 3.15)
project(ltm_native_tests LANGUAGES CXX)

# This CMake project builds and runs the *native* C++ test for the
# handover scorer logic, independent of Python. It intentionally does not
# build the pybind11 extension module; that is handled by `setup.py` /
# `pip install -e .` from the repository root so a full C++ toolchain plus
# CMake is only required when working on the native code itself.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(ltm_native_core STATIC
    src/mobility_score.cpp
)
target_include_directories(ltm_native_core PUBLIC include)

enable_testing()

add_executable(ltm_native_tests tests/test_mobility_score.cpp)
target_link_libraries(ltm_native_tests PRIVATE ltm_native_core)

add_test(NAME ltm_native_tests COMMAND ltm_native_tests)
