# usdl2

Native **SDL2 subset** for Python (`import usdl2`) — pure C for MicroPython (unix + windows), CircuitPython unix, and CPython (`python3` / `python.exe` / Android). Android APKs install native `android_21_*` wheels from TestPyPI via [pydisplay_android](https://github.com/PyDevices/pydisplay_android) (linked against the APK’s p4a SDL2 bootstrap). When this module is not linked/installed, pydisplay falls back to [`add_ons/usdl2.py`](https://github.com/PyDevices/pydisplay/blob/main/src/add_ons/usdl2.py).

Public names are **SDL2 symbols only** (functions, constants, macros such as `SDL_DEFINE_PIXELFORMAT`). Custom helpers do not belong here.

## Layout

```text
usdl2/
  micropython.mk / circuitpython.mk / setup.py   # build glue (stay at root)
  src/usdl2_mp.c                                 # MicroPython + CircuitPython
  src/usdl2_cpy.c                                # CPython Extension
  include/usdl2.h, usdl2_module_globals.inc, …
  test_usdl2.py
```

## SDL2 at build time

### Linux / unix

```bash
sudo apt install libsdl2-dev   # pkg-config sdl2
```

### Windows — two trees under `SDL2_DEV`

Do **not** vendor SDL2 in this repo. Download official devel ZIPs from [libsdl.org releases](https://github.com/libsdl-org/SDL/releases):

| Consumer | ZIP | Example unpack | `SDL2_DEV` |
|----------|-----|----------------|------------|
| `micropython.exe` (MinGW) | `SDL2-devel-*-mingw.zip` | `~/SDL2-2.30.10` (`x86_64-w64-mingw32/` inside) | that path |
| `python.exe` (MSVC) | `SDL2-devel-*-VC.zip` | `C:\SDL2-2.30.10-VC` (`include/` + `lib/x64/`) | that path |

`micropython.mk` links MinGW SDL2 **statically**. CPython wheels/extensions need `SDL2.dll` on `PATH` at runtime (from `lib/x64` of the VC tree).

From WSL, set a Windows-visible path before `pip.exe`:

```bash
# MSVC tree copied to the Windows drive for reliable builds
export SDL2_DEV='C:\SDL2-2.30.10-VC'
cmd.exe /c "set SDL2_DEV=$SDL2_DEV&& pip.exe install -e $(wslpath -w "$PWD")"
cmd.exe /c "set PATH=C:\SDL2-2.30.10-VC\lib\x64;%PATH%&& python.exe $(wslpath -w test_usdl2.py)"
```

See also [`scripts/sdl2_dev_env.sh`](scripts/sdl2_dev_env.sh).

## MicroPython

```bash
# unix
cd micropython/ports/unix && make USER_C_MODULES=../../..
# windows (MinGW)
export SDL2_DEV=~/SDL2-2.30.10
cd micropython/ports/windows && make USER_C_MODULES=../../..
```

Or from [cmods](https://github.com/PyDevices/cmods): `./build_mp.sh --port unix|windows --variant standard`.

```bash
./micropython/ports/unix/build-standard/micropython usdl2/test_usdl2.py
./micropython/ports/windows/build-standard/micropython.exe usdl2/test_usdl2.py
```

## CircuitPython (unix)

```bash
./apply_cp_unix_usdl_patches.sh --apply
../lv_circuitpython_mod/build_cp.sh --port unix --variant coverage
../circuitpython/ports/unix/build-coverage/micropython test_usdl2.py
```

## CPython

```bash
# unix
pip install -e .
xvfb-run -a python3 test_usdl2.py

# windows — see SDL2_DEV table above + pip.exe / python.exe
```

## Android

CI publishes `android_21_arm64_v8a` and `android_21_x86_64` wheels (`cp313` / `cp314`) to TestPyPI. Local reproduction:

```bash
export ANDROID_HOME=~/.buildozer/android/platform/android-sdk
export ANDROID_NDK_HOME=~/.buildozer/android/platform/android-ndk-r28c
echo "0.0.0.dev" > VERSION
./scripts/ci_prepare_sdl2_android.sh
pipx run cibuildwheel --platform android
```

APK packaging lives in [pydisplay_android](https://github.com/PyDevices/pydisplay_android).

## Smoke test

`test_usdl2.py` exercises init, window/renderer, packed rects, `SDL_Event` layout, timers, and `SDL_PumpEvents` on every runtime above.
