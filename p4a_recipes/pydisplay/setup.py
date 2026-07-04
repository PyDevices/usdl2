# SPDX-License-Identifier: MIT
"""Install pydisplay library packages from src/lib (used by p4a recipe)."""

from pathlib import Path

from setuptools import find_packages, setup

lib = Path(__file__).resolve().parent / "src" / "lib"

setup(
    name="pydisplay",
    version="0.1.0",
    packages=find_packages(where=str(lib)),
    package_dir={"": str(lib)},
    python_requires=">=3.8",
)
