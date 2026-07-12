# SPDX-License-Identifier: MIT
"""Build native usdl2 SDL2 C extension for CPython (unix, windows, Android)."""

import os
import subprocess
import sys

from setuptools import Extension, setup

ROOT = os.path.dirname(os.path.abspath(__file__))
USDL2_SOURCES = [os.path.join("src", "usdl2_cpy.c")]
INCLUDE_DIRS = [os.path.join(ROOT, "include")]


def _unix_sdl2():
    # Android / p4a: headers from bootstrap JNI; link SDL2 provided by the recipe env.
    android_include = os.environ.get("USDL2_SDL2_INCLUDE", "").strip()
    if os.environ.get("USDL2_ANDROID", "").strip() in ("1", "true", "yes") or android_include:
        include_dirs = INCLUDE_DIRS + ([android_include] if android_include else [])
        return include_dirs, [], ["SDL2"], [
            "-Wno-unused-function",
            "-Wno-sign-compare",
        ], []

    pkg_config = subprocess.run(
        ["pkg-config", "--cflags", "--libs", "sdl2"],
        capture_output=True,
        text=True,
        check=True,
    ).stdout.split()
    include_dirs = INCLUDE_DIRS + [f[2:] for f in pkg_config if f.startswith("-I")]
    library_dirs = [f[2:] for f in pkg_config if f.startswith("-L")]
    libraries = [f[2:] for f in pkg_config if f.startswith("-l")]
    extra_compile_args = [f for f in pkg_config if f.startswith("-D")] + [
        "-Wno-unused-function",
        "-Wno-sign-compare",
    ]
    return include_dirs, library_dirs, libraries, extra_compile_args, []


def _windows_sdl2():
    # SDL2_DEV: unpacked official SDL2 development ZIP.
    # - MSVC VC layout (python.org CPython): $SDL2_DEV/include + $SDL2_DEV/lib/x64
    # - MinGW layout (micropython.mk): $SDL2_DEV/<triplet>/include/SDL2
    sdl2_dev = os.environ.get("SDL2_DEV")
    if not sdl2_dev:
        raise SystemExit(
            "Building usdl2 on Windows requires SDL2_DEV pointing at an "
            "unpacked SDL2 development ZIP (see README.md)."
        )
    sdl2_dev = os.path.abspath(sdl2_dev)
    msvc_include = os.path.join(sdl2_dev, "include")
    msvc_lib = os.path.join(sdl2_dev, "lib", "x64")
    mingw_prefix = os.path.join(sdl2_dev, "x86_64-w64-mingw32")
    mingw_include = os.path.join(mingw_prefix, "include", "SDL2")
    mingw_lib = os.path.join(mingw_prefix, "lib")

    if os.path.isdir(msvc_include) and os.path.isdir(msvc_lib):
        include_dirs = INCLUDE_DIRS + [msvc_include]
        library_dirs = [msvc_lib]
    elif os.path.isdir(mingw_include) and os.path.isdir(mingw_lib):
        include_dirs = INCLUDE_DIRS + [os.path.join(mingw_prefix, "include"), mingw_include]
        library_dirs = [mingw_lib]
    else:
        raise SystemExit(
            f"SDL2_DEV={sdl2_dev!r} has neither MSVC (include/ + lib/x64/) "
            "nor MinGW (x86_64-w64-mingw32/) layout — see README.md."
        )

    return include_dirs, library_dirs, ["SDL2"], ["/wd4996"], []


if sys.platform == "win32":
    include_dirs, library_dirs, libraries, extra_compile_args, extra_link_args = _windows_sdl2()
else:
    include_dirs, library_dirs, libraries, extra_compile_args, extra_link_args = _unix_sdl2()

setup(
    name="usdl2",
    packages=[],
    py_modules=[],
    ext_modules=[
        Extension(
            "usdl2",
            sources=USDL2_SOURCES,
            include_dirs=include_dirs,
            library_dirs=library_dirs,
            libraries=libraries,
            extra_compile_args=extra_compile_args,
            extra_link_args=extra_link_args,
        ),
    ],
)
