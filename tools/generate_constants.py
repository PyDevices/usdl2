#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Regenerate python/usdl2/_constants.py from usdl2.h."""

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
text = (ROOT / "usdl2.h").read_text()
lines = []
for m in re.finditer(r"MP_QSTR_(SDL_\w+).*MP_ROM_INT\(([^)]+)\)", text):
    lines.append(f"{m.group(1)} = {m.group(2)}")
out = ROOT / "python" / "usdl2" / "_constants.py"
out.write_text(
    "# Auto-generated from usdl2.h — run tools/generate_constants.py\n"
    + "\n".join(lines)
    + "\n"
)
print(f"wrote {len(lines)} constants to {out}")
