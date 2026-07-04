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

`build_mp.sh` sets `SDL2_DEV` for the make step when it is exported in your shell. The official MinGW ZIP’s `sdl2.pc` files embed incorrect `prefix` paths from the SDL build machine; the build uses the unpacked `x86_64-w64-mingw32/` (or `i686-w64-mingw32/`) layout under `SDL2_DEV` directly instead of pkg-config.

Cross-compiling the windows port from Linux/WSL also needs MinGW-w64 (`sudo apt install gcc-mingw-w64`). `build_mp.sh` sets `CROSS_COMPILE` when appropriate.

The windows build links SDL2 **statically** so `micropython.exe` does not require `SDL2.dll` beside it.

## MicroPython (cmods)

Clone into the [cmods](https://github.com/PyDevices/cmods) workspace (sibling of `micropython/`):

```bash
git clone https://github.com/PyDevices/usdl2.git
```

**Unix:**

```bash
./build_mp.sh --port unix --variant standard
./micropython/ports/unix/build-standard/micropython ./usdl2/test_usdl2.py
```

**Windows** (from WSL/Linux with cross MinGW, after setting `SDL2_DEV`):

```bash
export SDL2_DEV=~/SDL2-2.30.10
./build_mp.sh --port windows --variant standard
# Run from WSL (or native Windows):
#   micropython/ports/windows/build-standard/micropython.exe usdl2/test_usdl2.py
```

No patching required — `micropython.mk` is picked up via `USER_C_MODULES`.

## CircuitPython

Requires sibling `circuitpython/` tree and patch script (unix only):

```bash
./apply_cp_unix_usdl_patches.sh --apply
./build_cp.sh --port unix --variant standard
./circuitpython/ports/unix/build-standard/micropython ./usdl2/test_usdl2.py
```

For coverage/gcov builds, use `--variant coverage` with both scripts.

## Patch script (CircuitPython only)

```bash
./apply_cp_unix_usdl_patches.sh --dry-run   # preview
./apply_cp_unix_usdl_patches.sh --apply     # write patches + copy spike
./apply_cp_unix_usdl_patches.sh --status    # check patch state
```

Embedded ports are unaffected (`CIRCUITPY_USDL2` defaults to 0 on CircuitPython).

## Android (CPython + python-for-android)

On Android there is no MicroPython port; pydisplay runs under **CPython** in a **python-for-android** APK with the **SDL2 bootstrap**. The same `import usdl2` API is provided by a **ctypes FFI package** in `python/usdl2/` (sets `_USE_FFI = True`). pydisplay's existing `SDLDisplay` backend works unchanged once `usdl2` is installed.

### Quick test (desktop, FFI)

```bash
pip install -e .
xvfb-run -a python3 test_usdl2.py   # headless CI / Linux without a display
```

### Build demo APK

Prerequisites: [Android SDK + NDK](https://python-for-android.readthedocs.io/en/latest/quickstart.html), `pip install buildozer`.

```bash
cd android_demo
./build_apk.sh
# APK: android_demo/bin/pydisplaydemo-0.2.0-*-debug.apk (name may vary)
adb install -r bin/*.apk
```

`build_apk.sh` sets `P4A_usdl2_DIR` to the repo root. If `../pydisplay` exists (sibling clone), it also sets `P4A_pydisplay_DIR` for an in-tree pydisplay build.

### pydisplay on Android

The demo APK uses **pydisplay** (`SDLDisplay`, `eventsys`, `multimer`) via the `pydisplay` p4a recipe:

| File | Role |
|------|------|
| `p4a_recipes/pydisplay/` | Installs `displaysys`, `eventsys`, `graphics`, `multimer` from pydisplay `src/lib/` |
| `android_demo/board_config.py` | SDL display + event broker (landscape, fullscreen on Android) |
| `android_demo/main.py` | Touch-paint demo using pydisplay APIs |
| `android_demo/main_usdl2_raw.py` | Raw `usdl2` reference demo (no pydisplay) |

`buildozer.spec` requirements: `python3,sdl2,usdl2,pydisplay`.

Desktop smoke test (Xvfb, requires sibling `pydisplay` clone):

```bash
git clone https://github.com/PyDevices/pydisplay.git ../pydisplay
cd android_demo && ./test_desktop.sh
```

For your own app, copy `board_config.py`, add `pydisplay` + `usdl2` to `buildozer.spec`, and write your main loop with `display_drv` / `broker` as usual.

## Layout

| File | Role |
|------|------|
| `usdl2.c`, `usdl2.h`, `usdl2_module_globals.inc` | Native module: SDL bindings, constants, events, timers |
| `python/usdl2/` | CPython ctypes implementation (`pip install -e .`, Android p4a) |
| `setup.py` | Editable install for CPython / p4a |
| `p4a_recipes/usdl2/` | python-for-android recipe (depends on `sdl2`) |
| `p4a_recipes/pydisplay/` | python-for-android recipe (depends on `usdl2`) |
| `android_demo/` | pydisplay touch-paint APK + raw usdl2 example |
| `micropython.mk` | MicroPython user C module glue (`MP_REGISTER_MODULE` in `usdl2.c`) |
| `circuitpython.mk` | CircuitPython port Makefile fragment (`usdl2.c` only; module registration in `shared-bindings/usdl2/__init.c`) |
| `circuitpython_spike/` | Templates copied into CircuitPython tree |
| `test_usdl2.py` | Smoke test (MicroPython native or CPython FFI) |
