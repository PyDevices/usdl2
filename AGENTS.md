# AGENTS.md — usdl2

Native **SDL2 subset for Python** (`import usdl2`). Pure C for MicroPython,
CircuitPython, CPython, and Android (p4a). When this module is not linked,
pydisplay falls back to `src/add_ons/usdl2.py` (same public contract).

## Hard rule: SDL2 symbols only

**usdl2 must only expose symbols that exist in SDL2** (functions, macros,
constants, and binding-shaped constructors for SDL types such as
`SDL_Rect` / `SDL_Point` / `SDL_Event` / `SDL_TimerCallback`).

**Do not** add custom helpers, wrappers, or “convenience” APIs to this module.
Past mistakes to never repeat:

| Forbidden (not in SDL2) | Why it appeared | Where it belongs instead |
|-------------------------|-----------------|---------------------------|
| `process_exit` / hard process kill | Test-kit / display teardown | Caller (`sdldisplay._hard_process_exit`, `os._exit`, libc `_exit`) |
| `pump_scheduler` | MP cooperative timer delivery | Not a public API — MP/CP may drain the scheduler **inside** real SDL entry points such as `SDL_PumpEvents` / `SDL_PollEvent` as an implementation detail only |
| Bare `Event` (non-`SDL_` name) | Shorthand type export | Export the type as **`SDL_Event` only** |
| `_USE_FFI` as a public contract | Detect ctypes vs native | Prefer matching object shapes; do not invent new module attributes for consumers |

If a consumer needs something that is not an SDL2 symbol, **fix the consumer**
or put the helper in the consumer package — never grow usdl2’s public surface.

When adding a binding, check the SDL2 docs/headers first. If the name is not
`SDL_*` (or an established SDL macro like `SDL_DEFINE_PIXELFORMAT`), it does
not belong here.

## Layout

- Root: `micropython.mk`, `circuitpython.mk`, `setup.py`, patch scripts
- `src/usdl2_mp.c` — MicroPython + CircuitPython
- `src/usdl2_cpy.c` — CPython extension
- `include/` — shared headers / `usdl2_module_globals.inc` / qstrs
- No `.c` / `.h` / `.inc` at repo root; no ctypes `python/` package

## Smoke

`test_usdl2.py` on MP unix, MP windows, CP unix, CPython unix, and `python.exe`
(see README for `SDL2_DEV`). Do not weaken tests to allow non-SDL public names.
