# usdl2

Unix-only CircuitPython native module: a **subset** of libSDL2 for pydisplay (`import usdl2`).

## Setup

Requires sibling `circuitpython/` tree and `libsdl2-dev`:

```bash
sudo apt install libsdl2-dev   # Debian/Ubuntu
./apply_cp_unix_usdl_patches.sh --apply
../lv_circuitpython_mod/build_cp_unix.sh
```

## Smoke test

```bash
../circuitpython/ports/unix/build-coverage/micropython ./test_usdl2_cp_unix.py
```

## Patch script

```bash
./apply_cp_unix_usdl_patches.sh --dry-run   # preview
./apply_cp_unix_usdl_patches.sh --apply     # write patches + copy spike
./apply_cp_unix_usdl_patches.sh --status    # check patch state
```

Embedded CP ports are unaffected (`CIRCUITPY_USDL2` defaults to 0).
