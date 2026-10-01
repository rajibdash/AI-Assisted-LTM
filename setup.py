"""Build configuration for the optional `ltm_native` pybind11 extension.

Project metadata (name, version, dependencies, entry points, ...) lives in
`pyproject.toml`; this file only adds the native extension module, which
cannot currently be expressed declaratively in `pyproject.toml`.

Build with:

    python -m pip install -e .

The extension sources live under `native/` (see `native/CMakeLists.txt` for
the native-only build used to run the C++ tests independent of Python).
"""

from pybind11.setup_helpers import Pybind11Extension, build_ext
from setuptools import setup

ext_modules = [
    Pybind11Extension(
        "ltm_native",
        sources=[
            "native/src/bindings.cpp",
            "native/src/mobility_score.cpp",
        ],
        include_dirs=["native/include"],
        cxx_std=17,
    )
]

setup(
    ext_modules=ext_modules,
    cmdclass={"build_ext": build_ext},
)
