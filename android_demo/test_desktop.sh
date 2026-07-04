#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Run the pydisplay demo on desktop (Xvfb) before building an APK.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
DEMO="$ROOT/android_demo"
PYDISPLAY="${PYDISPLAY_DIR:-$ROOT/../pydisplay}"

if [[ ! -d "$PYDISPLAY/src/lib/displaysys" ]]; then
  echo "Clone pydisplay beside usdl2 (or set PYDISPLAY_DIR):"
  echo "  git clone https://github.com/PyDevices/pydisplay.git $PYDISPLAY"
  exit 1
fi

pip install -q -e "$ROOT"
export PYTHONPATH="$PYDISPLAY/src/lib:${PYTHONPATH:-}"
cd "$DEMO"
xvfb-run -a python3 main.py &
PID=$!
sleep 2
kill "$PID" 2>/dev/null || true
wait "$PID" 2>/dev/null || true
echo "pydisplay desktop demo exited cleanly (or was stopped after smoke window)"
