# SPDX-License-Identifier: MIT
"""Smoke test for usdl2 (MicroPython unix/windows or CircuitPython unix)."""

import struct
import usdl2

# SDL_INIT_VIDEO
assert usdl2.init(0x00000020) == 0

# Joystick subsystem API (parity with pydisplay sdldisplay PR #37)
assert usdl2.init_subsystem(0x00000200) == 0
assert isinstance(usdl2.num_joysticks(), int)

win = usdl2.create_window("usdl2 test", 100, 100, 320, 240, 0)
assert win

renderer = usdl2.create_renderer(win, -1, 0x00000002)
assert renderer

usdl2.set_render_draw_color(renderer, 32, 64, 128, 255)
usdl2.render_clear(renderer)
usdl2.render_present(renderer)

# Synthetic joystick events: verify Event subviews match SDL2 layout.
def _pack_event(event_type, payload):
    buf = bytearray(56)
    struct.pack_into("<I", buf, 0, event_type)
    buf[8:8 + len(payload)] = payload
    return usdl2.Event(buf)


axis_evt = _pack_event(
    0x600,
    struct.pack("<iBxxxh", 1, 2, -16384),
)
assert axis_evt.jaxis.which == 1
assert axis_evt.jaxis.axis == 2
assert axis_evt.jaxis.value == -16384

ball_evt = _pack_event(
    0x601,
    struct.pack("<iBxxxhh", 3, 1, 10, -5),
)
assert ball_evt.jball.which == 3
assert ball_evt.jball.ball == 1
assert ball_evt.jball.xrel == 10
assert ball_evt.jball.yrel == -5

hat_evt = _pack_event(
    0x602,
    struct.pack("<iBBxx", 4, 0, 0x05),
)
assert hat_evt.jhat.which == 4
assert hat_evt.jhat.hat == 0
assert hat_evt.jhat.value == 0x05

btn_evt = _pack_event(
    0x603,
    struct.pack("<iBBBB", 5, 7, 1, 0),
)
assert btn_evt.jbutton.which == 5
assert btn_evt.jbutton.button == 7
assert btn_evt.jbutton.state == 1

event = usdl2.Event()
while usdl2.poll_event(event):
    if event.type == 0x100:
        break

usdl2.destroy_renderer(renderer)
usdl2.destroy_window(win)
usdl2.quit()

print("usdl2 smoke test ok")
