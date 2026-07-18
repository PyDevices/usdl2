#!/usr/bin/env python3
"""Stress usdl2 SDL timers vs micropython.schedule queue (host runner).

Arms several fast periodic timers, busy-waits without pumping, then drains via
SDL_PumpEvents. Counts \"SDL timer schedule queue full\" on stderr.
Writes NDJSON evidence to the debug log when present.
"""

from __future__ import annotations

import json
import os
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
DEBUG_LOG = Path("/home/brad/gh/pydevices/cmods/.cursor/debug-2e38c1.log")
SESSION = "2e38c1"

MP_SCRIPT = r"""
import sys
try:
    sys.path.remove("")
except ValueError:
    pass
import usdl2
from usdl2 import SDL_Init, SDL_INIT_TIMER, SDL_AddTimer, SDL_RemoveTimer, SDL_TimerCallback, SDL_PumpEvents

SDL_Init(SDL_INIT_TIMER)
hits = [0]

def cb(interval, param):
    hits[0] += 1
    return interval

cbs = []
ids = []
for i in range(4):
    c = SDL_TimerCallback(cb)
    cbs.append(c)
    tid = SDL_AddTimer(5, c, None)
    ids.append(tid)
    print("armed", i, bool(tid))

# Starve the VM of PumpEvents while SDL timer thread fires.
t0 = __import__("time").ticks_ms() if hasattr(__import__("time"), "ticks_ms") else None
busy_ms = 400
if t0 is not None:
    while __import__("time").ticks_diff(__import__("time").ticks_ms(), t0) < busy_ms:
        pass
else:
    __import__("time").sleep(busy_ms / 1000)

# Drain
for _ in range(50):
    SDL_PumpEvents()

for tid in ids:
    if tid:
        SDL_RemoveTimer(tid)
print("HITS", hits[0])
print("STRESS_OK")
"""


def log(msg: str, data: dict) -> None:
    rec = {
        "sessionId": SESSION,
        "runId": "sched-stress",
        "hypothesisId": "H1",
        "location": "stress_sdl_timer_sched.py",
        "message": msg,
        "data": data,
        "timestamp": int(time.time() * 1000),
    }
    DEBUG_LOG.parent.mkdir(parents=True, exist_ok=True)
    with DEBUG_LOG.open("a") as f:
        f.write(json.dumps(rec) + "\n")


def main() -> int:
    exe = Path(os.environ.get("MICROPY_MICROPYTHON", ""))
    if not exe.is_file():
        candidates = [
            ROOT / "micropython/ports/windows/build-dev/micropython.exe",
            ROOT.parent / "pydisplay/bin/micropython.exe",
            ROOT / "micropython/ports/unix/build-standard/micropython",
        ]
        for c in candidates:
            if c.is_file():
                exe = c
                break
    if not exe.is_file():
        print("missing micropython binary", file=sys.stderr)
        return 2

    env = os.environ.copy()
    env.setdefault("SDL_VIDEODRIVER", "dummy")
    env.setdefault("SDL_AUDIODRIVER", "dummy")

    p = subprocess.run(
        [str(exe), "-c", MP_SCRIPT],
        capture_output=True,
        text=True,
        env=env,
        timeout=30,
    )
    out = (p.stdout or "") + (p.stderr or "")
    full_n = out.count("SDL timer schedule queue full")
    ok = p.returncode == 0 and "STRESS_OK" in out
    print(out)
    print(f"queue_full_lines={full_n} rc={p.returncode} ok={ok} exe={exe}")
    log(
        "stress complete",
        {"queue_full_lines": full_n, "rc": p.returncode, "ok": ok, "exe": str(exe), "tail": out[-800:]},
    )
    # Success: ran cleanly with at most one rate-limited full line (or zero).
    return 0 if ok and full_n <= 1 else 1


if __name__ == "__main__":
    raise SystemExit(main())
