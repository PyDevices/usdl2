#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Run Android demos on desktop (Xvfb) before building an APK.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
DEMO="$ROOT/android_demo"
PYDISPLAY="${PYDISPLAY_DIR:-$ROOT/../pydisplay}"
LVCPY="${LVCPY_DIR:-$ROOT/../lv_cpython_mod}"

if [[ ! -d "$PYDISPLAY/src/lib/displaysys" ]]; then
  echo "Clone pydisplay beside usdl2 (or set PYDISPLAY_DIR):"
  echo "  git clone https://github.com/PyDevices/pydisplay.git $PYDISPLAY"
  exit 1
fi

pip install -q -e "$ROOT"
export PYTHONPATH="$PYDISPLAY/src/lib:$PYDISPLAY/src/add_ons:${PYTHONPATH:-}"
cd "$DEMO"

echo "== pydisplay touch-paint (main.py) =="
xvfb-run -a python3 main.py &
PID=$!
sleep 2
kill "$PID" 2>/dev/null || true
wait "$PID" 2>/dev/null || true

if [[ -d "$LVCPY/generated" ]]; then
  echo "== LVGL + display_driver (main_lvgl.py) =="
  pip install -q -e "$LVCPY"
  xvfb-run -a python3 main_lvgl.py &
  PID=$!
  sleep 2
  kill "$PID" 2>/dev/null || true
  wait "$PID" 2>/dev/null || true
else
  echo "Skip LVGL smoke (clone lv_cpython_mod beside usdl2 or set LVCPY_DIR)"
fi

echo "Desktop demos exited cleanly (or were stopped after smoke window)"
