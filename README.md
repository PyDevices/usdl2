# usdl2

Unix-only native module: a **subset** of libSDL2 for pydisplay (`import usdl2`). Works on MicroPython and CircuitPython unix builds.

Requires `libsdl2-dev`:

```bash
sudo apt install libsdl2-dev   # Debian/Ubuntu
```

## MicroPython (cmods)

Clone into the [cmods](https://github.com/PyDevices/cmods) workspace (sibling of `micropython/`):

```bash
git clone https://github.com/PyDevices/usdl2.git
./build_mp.sh --port unix --variant standard
./micropython/ports/unix/build-standard/micropython ./usdl2/test_usdl2.py
```

No patching required — `micropython.mk` is picked up via `USER_C_MODULES`.

## CircuitPython

Requires sibling `circuitpython/` tree and patch script:

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

## Layout

| File | Role |
|------|------|
| `usdl2.h`, `usdl2_wrappers.c`, `usdl2_event.c` | Shared SDL bindings |
| `modusdl2.c` | Module registration (both runtimes) |
| `micropython.mk` | MicroPython user C module glue |
| `circuitpython.mk` | CircuitPython port Makefile fragment |
| `circuitpython_spike/` | Templates copied into CircuitPython tree |
| `test_usdl2.py` | Smoke test (both runtimes) |
