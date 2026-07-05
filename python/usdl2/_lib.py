# SPDX-License-Identifier: MIT
"""Load libSDL2 for CPython (desktop and Android p4a)."""

from __future__ import annotations

import ctypes
import sys


def load_sdl2():
    names = []
    if sys.platform == "android":
        names.append("libSDL2.so")
    elif sys.platform == "win32":
        names.extend(("SDL2", "libSDL2"))
    elif sys.platform == "darwin":
        names.extend(("libSDL2-2.0.0.dylib", "SDL2", "libSDL2.dylib"))
    else:
        names.extend(("SDL2", "libSDL2.so.0", "libSDL2.so"))

    last_err = None
    for name in names:
        try:
            return ctypes.CDLL(name)
        except OSError as exc:
            last_err = exc
    raise OSError(f"Could not load SDL2 ({names!r}): {last_err}")


_sdl = None


def sdl():
    global _sdl
    if _sdl is None:
        _sdl = load_sdl2()
    return _sdl
