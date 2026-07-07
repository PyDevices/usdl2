# SPDX-License-Identifier: MIT
from setuptools import find_packages, setup

setup(
    name="usdl2",
    version="0.0.1",
    description="CPython ctypes SDL2 subset for pydisplay (Android and desktop)",
    license="MIT",
    packages=find_packages(where="python"),
    package_dir={"": "python"},
    python_requires=">=3.8",
    classifiers=[
        "Programming Language :: Python :: 3",
        "Operating System :: OS Independent",
    ],
)
