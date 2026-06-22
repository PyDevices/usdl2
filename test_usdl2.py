# SPDX-License-Identifier: MIT
"""Smoke test for usdl2 (MicroPython unix/windows or CircuitPython unix)."""

import usdl2

# SDL_INIT_VIDEO
assert usdl2.init(0x00000020) == 0

win = usdl2.create_window("usdl2 test", 100, 100, 320, 240, 0)
assert win

renderer = usdl2.create_renderer(win, -1, 0x00000002)
assert renderer

usdl2.set_render_draw_color(renderer, 32, 64, 128, 255)
usdl2.render_clear(renderer)
usdl2.render_present(renderer)

event = usdl2.Event()
while usdl2.poll_event(event):
    if event.type == 0x100:
        break

usdl2.destroy_renderer(renderer)
usdl2.destroy_window(win)
usdl2.quit()

print("usdl2 smoke test ok")
