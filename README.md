# usdl2

Desktop native module: a **subset** of libSDL2 for pydisplay (`import usdl2`). Works on MicroPython **unix** and **windows** ports, and CircuitPython unix builds.

The module exports the same SDL2-style names as pydisplay's `displaysys.sdldisplay._sdl2` (`SDL_Init`, `SDL_CreateWindow`, `SDL_Rect`, `SDL_Event`, constants, timers, etc.) from a single native `usdl2` built-in module. pydisplay loads it first in both **`displaysys.sdldisplay._sdl2`** (display, input, rendering) and **`multimer._sdl2`** (SDL timer backend on CPython when POSIX and threading backends are unavailable).

## SDL2 at build time

### Linux / unix port

```bash
sudo apt install libsdl2-dev   # Debian/Ubuntu
```

### Windows port (MinGW)

Do **not** vendor SDL2 in this repo. Download the official **SDL2 MinGW development ZIP** from [libsdl.org](https://github.com/libsdl-org/SDL/releases) (asset name like `SDL2-devel-2.x.x-mingw.zip`), unpack it somewhere outside the repo, and point the build at it:

```bash
# Example: unpacked to ~/SDL2-2.30.10 (contains x86_64-w64-mingw32/ and i686-w64-mingw32/)
export SDL2_DEV=~/SDL2-2.30.10
```

Export `SDL2_DEV` before `make` so `micropython.mk` can find the headers/libs. The official MinGW ZIP’s `sdl2.pc` files embed incorrect `prefix` paths from the SDL build machine; the build uses the unpacked `x86_64-w64-mingw32/` (or `i686-w64-mingw32/`) layout under `SDL2_DEV` directly instead of pkg-config.

Cross-compiling the windows port from Linux/WSL also needs MinGW-w64 (`sudo apt install gcc-mingw-w64`) and a suitable `CROSS_COMPILE` (e.g. `x86_64-w64-mingw32-`).

The windows build links SDL2 **statically** so `micropython.exe` does not require `SDL2.dll` beside it.

## MicroPython

Clone as a sibling of `micropython/`:

```
workspace/
  usdl2/          ← this repo
  micropython/
```

```bash
git clone https://github.com/PyDevices/usdl2.git
```

**Unix:**

```bash
cd micropython/ports/unix
make submodules
make USER_C_MODULES=../../..
cd ../../..
./micropython/ports/unix/build-standard/micropython usdl2/test_usdl2.py
```

**Windows** (from WSL/Linux with cross MinGW, after setting `SDL2_DEV`):

```bash
export SDL2_DEV=~/SDL2-2.30.10
cd micropython/ports/windows
make submodules
make USER_C_MODULES=../../..
cd ../../..
# Run from WSL (or native Windows):
#   micropython/ports/windows/build-standard/micropython.exe usdl2/test_usdl2.py
```

No patching required — `micropython.mk` is picked up via `USER_C_MODULES` (the workspace directory containing this repo).

([cmods](https://github.com/PyDevices/cmods) is an optional convenience workspace with `./build_mp.sh`; it is not required.)
## CircuitPython

Requires sibling `circuitpython/` and `lv_circuitpython_mod/` trees (unix only) — `apply_cp_unix_usdl_patches.sh` patches usdl2 into the CircuitPython tree, `lv_circuitpython_mod/build_cp.sh` drives the actual `make`:

```
workspace/
  usdl2/                  ← this repo
  circuitpython/
  lv_circuitpython_mod/
```

```bash
cd usdl2
./apply_cp_unix_usdl_patches.sh --apply
../lv_circuitpython_mod/build_cp.sh --port unix --variant standard
cd ..
./circuitpython/ports/unix/build-standard/micropython usdl2/test_usdl2.py
```

For coverage/gcov builds, use `--variant coverage` with both scripts.

## Patch script (CircuitPython only)

```bash
./apply_cp_unix_usdl_patches.sh --dry-run   # preview
./apply_cp_unix_usdl_patches.sh --apply     # write patches + copy spike
./apply_cp_unix_usdl_patches.sh --status    # check patch state
```

Embedded ports are unaffected (`CIRCUITPY_USDL2` defaults to 0 on CircuitPython).

## CPython / Android FFI

On Android there is no MicroPython port; the same `import usdl2` API is provided by a **ctypes FFI package** in `python/usdl2/` (sets `_USE_FFI = True`). pydisplay's existing `SDLDisplay` backend works unchanged once `usdl2` is installed.

**Desktop FFI smoke test:**

```bash
pip install -e .
xvfb-run -a python3 test_usdl2.py   # headless CI / Linux without a display
```

**Android APK builds** (pydisplay + LVGL demos, buildozer, p4a recipes) live in the separate [**pydisplay_android**](https://github.com/PyDevices/pydisplay_android) repo. This repo keeps only the ctypes package and `p4a_recipes/usdl2/` for python-for-android.

## Layout

| File | Role |
|------|------|
| `usdl2.c`, `usdl2.h`, `usdl2_module_globals.inc` | Native module: SDL bindings, constants, events, timers |
| `python/usdl2/` | CPython ctypes implementation (`pip install -e .`, Android p4a) |
| `setup.py` | Editable install for CPython / p4a |
| `p4a_recipes/usdl2/` | python-for-android recipe (depends on `sdl2`) |
| `micropython.mk` | MicroPython user C module glue (`MP_REGISTER_MODULE` in `usdl2.c`) |
| `circuitpython.mk` | CircuitPython port Makefile fragment (`usdl2.c` only; module registration in `shared-bindings/usdl2/__init.c`) |
| `circuitpython_spike/` | Templates copied into CircuitPython tree |
| `test_usdl2.py` | Smoke test (MicroPython native or CPython FFI) |
