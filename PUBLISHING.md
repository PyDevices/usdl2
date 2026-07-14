# Publishing and releases

How changes in this repo become versioned **`usdl2`** wheels on [TestPyPI](https://test.pypi.org/project/usdl2/), and how to install them.

The CPython package ships a native extension (`src/usdl2_cpy.c`) linked against libSDL2. CI builds platform wheels with [cibuildwheel](https://cibuildwheel.pypa.io/):

| Platform | Wheel tag |
|----------|-----------|
| Linux x86_64 | `manylinux_*` |
| Windows x64 | `win_amd64` |
| Android arm64 | `android_21_arm64_v8a` (`cp313`, `cp314`) |
| Android x86_64 (emulator) | `android_21_x86_64` (`cp313`, `cp314`) |

Android wheels link against SDL2 prepared in CI (`scripts/ci_prepare_sdl2_android.sh`) but do **not** vendor `libSDL2.so` — the p4a SDL2 bootstrap provides it inside the APK.

MicroPython / CircuitPython consume `src/usdl2_mp.c` via `micropython.mk` / `circuitpython.mk` (not the PyPI wheel).

APK packaging and p4a recipes live in [pydisplay_android](https://github.com/PyDevices/pydisplay_android).

When the native module is unavailable, pydisplay uses `src/add_ons/usdl2.py` as a pure-Python fallback — that file lives in pydisplay, not this repo.

## Pipeline overview

```text
usdl2 (your machine)
  commit → push main
           │
           ▼
  ./scripts/publish_release_tag.sh --push   (or manual git tag vX.Y.Z)
           │
           ▼
usdl2: Publish TestPyPI
  cibuildwheel → Linux + Windows + Android wheels → twine upload
```

## Version numbers

Format: **`X.Y.Z`** (semver). Later releases use the highest existing tag + 1 patch (`v0.0.7` → `0.0.8`, …).

```bash
./scripts/next_release_version.sh --verbose
```

TestPyPI rejects re-uploading the same version — each release needs a new tag.

## Release (local clone)

```bash
git push origin main
./scripts/publish_release_tag.sh --push
```

## Local wheel builds (cibuildwheel)

```bash
pipx install cibuildwheel
echo "0.0.0.dev" > VERSION
pipx run cibuildwheel --platform linux    # Docker
```

**Windows:** set `SDL2_DEV` to an unpacked [SDL2 VC ZIP](https://github.com/libsdl-org/SDL/releases).

**Android:** needs Android SDK + NDK:

```bash
export ANDROID_HOME=~/.buildozer/android/platform/android-sdk
export ANDROID_NDK_HOME=~/.buildozer/android/platform/android-ndk-r28c
echo "0.0.0.dev" > VERSION
pipx run cibuildwheel --platform android
ls wheelhouse/*android*.whl
```

## Install from TestPyPI

```bash
pip install -i https://test.pypi.org/simple/ usdl2
```
