#!/usr/bin/env python3
"""Apply usdl2 unix REPL stdin poll patch to CircuitPython ports/unix/unix_mphal.c."""

from __future__ import annotations

import sys
from pathlib import Path

MARKER = "usdl2-cmod begin (apply_cp_unix_usdl_patches.sh)"
POLL_INCLUDE = "#ifndef _WIN32\n#include <poll.h>\n#endif"

OLD_READ_BLOCK = """    unsigned char c;
    ssize_t ret;
    MP_HAL_RETRY_SYSCALL(ret, read(STDIN_FILENO, &c, 1), {});
    if (ret == 0) {
        c = 4; // EOF, ctrl-D
    } else if (c == '\n') {
        c = '\r';
    }
    return c;
}"""

NEW_READ_BLOCK = """    unsigned char c;
    #ifndef _WIN32
    // >>> usdl2-cmod begin (apply_cp_unix_usdl_patches.sh)
    // Poll scheduled callbacks (e.g. usdl2 SDL timers) while waiting for REPL input.
    for (;;) {
    for (int drain = 0; drain < 8; ++drain) {
        mp_event_handle_nowait();
    }
        struct pollfd pfd = { .fd = STDIN_FILENO, .events = POLLIN };
        int pret = poll(&pfd, 1, 0);
        if (pret > 0 && (pfd.revents & POLLIN)) {
            ssize_t ret;
            MP_HAL_RETRY_SYSCALL(ret, read(STDIN_FILENO, &c, 1), {});
            if (ret == 0) {
                c = 4; // EOF, ctrl-D
            } else if (c == '\n') {
                c = '\r';
            }
            return c;
        }
        mp_hal_delay_ms(1);
    }
    // >>> usdl2-cmod end
    #else
    ssize_t ret;
    MP_HAL_RETRY_SYSCALL(ret, read(STDIN_FILENO, &c, 1), {});
    if (ret == 0) {
        c = 4; // EOF, ctrl-D
    } else if (c == '\n') {
        c = '\r';
    }
    return c;
    #endif
}"""


def apply(path: Path, dry_run: bool = False) -> str:
    text = path.read_text()
    if MARKER in text:
        return "skip"
    if OLD_READ_BLOCK not in text and NEW_READ_BLOCK not in text:
        raise SystemExit(f"unix_mphal.c read loop anchor not found in {path}")

    if OLD_READ_BLOCK in text:
        text = text.replace(OLD_READ_BLOCK, NEW_READ_BLOCK, 1)
    if "#include <poll.h>" not in text:
        anchor = '#include <fcntl.h>'
        if anchor not in text:
            raise SystemExit(f"{anchor} not found in {path}")
        text = text.replace(anchor, anchor + "\n" + POLL_INCLUDE, 1)

    if dry_run:
        return "dry-run"
    path.write_text(text)
    return "patched"


def main() -> None:
    dry_run = "--dry-run" in sys.argv
    path = Path(sys.argv[-1])
    result = apply(path, dry_run=dry_run)
    print(result)


if __name__ == "__main__":
    main()
