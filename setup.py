# SPDX-License-Identifier: MIT
"""Build native usdl2 SDL2 C extension for CPython (unix, windows, Android)."""

import os
import subprocess
import sys
import sysconfig

from setuptools import Extension, setup

ROOT = os.path.dirname(os.path.abspath(__file__))
USDL2_SOURCES = [os.path.join("src", "usdl2_cpy.c")]
INCLUDE_DIRS = [os.path.join(ROOT, "include")]


def _truthy(name: str) -> bool:
    return os.environ.get(name, "").strip().lower() in ("1", "true", "yes", "on")


def _is_android() -> bool:
    if _truthy("USDL2_ANDROID"):
        return True
    if getattr(sys, "platform", "") == "android":
        return True
    plat = sysconfig.get_platform().replace("-", "_")
    return plat.startswith("android")


def _android_abi() -> str:
    plat = sysconfig.get_platform().replace("-", "_")
    if "arm64_v8a" in plat:
        return "arm64_v8a"
    if "x86_64" in plat:
        return "x86_64"
    raise SystemExit(
        f"usdl2 Android build: unsupported platform tag {sysconfig.get_platform()!r}"
    )


def _android_sdl2():
    # Prefixed trees from scripts/ci_prepare_sdl2_android.sh (link only; do not vendor).
    include = os.environ.get("USDL2_SDL2_INCLUDE", "").strip()
    lib = os.environ.get("USDL2_SDL2_LIB", "").strip()
    if not include or not lib:
        root = os.environ.get(
            "USDL2_SDL2_ANDROID_ROOT", os.path.join(ROOT, ".sdl2-android")
        )
        abi = _android_abi()
        include = include or os.path.join(root, abi, "include", "SDL2")
        lib = lib or os.path.join(root, abi, "lib")
    if not os.path.isfile(os.path.join(include, "SDL.h")):
        raise SystemExit(
            f"usdl2 Android build: SDL.h not found under {include!r} "
            "(run scripts/ci_prepare_sdl2_android.sh)"
        )
    if not os.path.isfile(os.path.join(lib, "libSDL2.so")):
        raise SystemExit(
            f"usdl2 Android build: libSDL2.so not found under {lib!r} "
            "(run scripts/ci_prepare_sdl2_android.sh)"
        )
    return (
        INCLUDE_DIRS + [include],
        [lib],
        ["SDL2"],
        ["-Wno-unused-function", "-Wno-sign-compare"],
        [],
    )


def _unix_sdl2():
    try:
        pkg_config = subprocess.run(
            ["pkg-config", "--cflags", "--libs", "sdl2"],
            capture_output=True,
            text=True,
            check=True,
        ).stdout.split()
    except (subprocess.CalledProcessError, FileNotFoundError) as exc:
        # Allow sdist/metadata without system SDL2; cibuildwheel Linux installs it.
        if _truthy("CIBUILDWHEEL") or _truthy("USDL2_REQUIRE_SDL2"):
            raise SystemExit(
                "Building usdl2 requires pkg-config sdl2 (install libsdl2-dev)"
            ) from exc
        return (
            INCLUDE_DIRS,
            [],
            ["SDL2"],
            ["-Wno-unused-function", "-Wno-sign-compare"],
            [],
        )
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


if _is_android():
    include_dirs, library_dirs, libraries, extra_compile_args, extra_link_args = (
        _android_sdl2()
    )
elif sys.platform == "win32":
    include_dirs, library_dirs, libraries, extra_compile_args, extra_link_args = (
        _windows_sdl2()
    )
else:
    include_dirs, library_dirs, libraries, extra_compile_args, extra_link_args = (
        _unix_sdl2()
    )

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
