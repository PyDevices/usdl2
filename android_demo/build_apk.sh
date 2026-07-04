#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Build the Android demo APK (requires Android SDK/NDK + buildozer).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT/android_demo"

if ! command -v buildozer >/dev/null 2>&1; then
  echo "Install buildozer: pip install buildozer"
  echo "Set ANDROID_HOME / ANDROID_NDK_HOME per python-for-android docs."
  exit 1
fi

# Use in-tree recipes instead of downloading from GitHub.
export P4A_usdl2_DIR="$ROOT"
if [[ -d "$ROOT/../pydisplay" ]]; then
  export P4A_pydisplay_DIR="$ROOT/../pydisplay"
fi

buildozer android debug "$@"
