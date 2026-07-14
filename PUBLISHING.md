# Publishing and releases

How changes in this repo become a versioned **`usdl2`** package on [TestPyPI](https://test.pypi.org/project/usdl2/), and how to install it.

The CPython package is a **native C extension** (`src/usdl2_cpy.c`, linked against libSDL2). Wheels are platform- and CPython-ABI-specific (not `py3-none-any`). GitHub Actions should build with `cibuildwheel` (see `pyproject.toml`) plus an sdist and upload with twine. Linux builders need `libsdl2-dev`; Windows builders need `SDL2_DEV` pointing at an unpacked **VC** development ZIP.

MicroPython / CircuitPython consume `src/usdl2_mp.c` via `micropython.mk` / `circuitpython.mk` (not the PyPI wheel).

Android APK packaging and p4a recipes live in [pydisplay_android](https://github.com/PyDevices/pydisplay_android) (installs this package from TestPyPI).

When the native module is unavailable, pydisplay uses `src/add_ons/usdl2.py` as a pure-Python fallback — that file lives in pydisplay, not this repo.
