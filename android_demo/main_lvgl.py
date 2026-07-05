# SPDX-License-Identifier: MIT
"""
LVGL touch demo for Android — wired through pydisplay display_driver.py.

Uses SDLDisplay (board_config), lv_utils event loop, and multimer._sdl2 on Android.
"""

import display_driver
import lvgl as lv
from multimer import needs_pump, pump, sleep_ms


def _build_ui():
    style_default = lv.style_t()
    style_default.init()
    style_default.set_width(lv.pct(33))
    style_default.set_height(lv.pct(33))
    style_default.set_bg_color(lv.palette_main(lv.PALETTE.BLUE))

    style_pressed = lv.style_t()
    style_pressed.init()
    style_pressed.set_transform_width(-10)
    style_pressed.set_transform_height(-10)
    style_pressed.set_bg_color(lv.palette_main(lv.PALETTE.GREEN))

    parent = lv.screen_active()
    alignments = (
        lv.ALIGN.TOP_LEFT,
        lv.ALIGN.TOP_MID,
        lv.ALIGN.TOP_RIGHT,
        lv.ALIGN.LEFT_MID,
        lv.ALIGN.CENTER,
        lv.ALIGN.RIGHT_MID,
        lv.ALIGN.BOTTOM_LEFT,
        lv.ALIGN.BOTTOM_MID,
        lv.ALIGN.BOTTOM_RIGHT,
    )
    for i, alignment in enumerate(alignments, start=1):
        btn = lv.button(parent)
        btn.align(alignment, 0, 0)
        btn.add_style(style_default, 0)
        btn.add_style(style_pressed, lv.STATE.PRESSED)
        label = lv.label(btn)
        label.set_text(f"Btn{i}")
        label.center()
        if needs_pump():
            pump()


_build_ui()
display_driver.run()

from board_config import broker
from eventsys import poll_quit_discarding_others

while True:
    pump()
    if poll_quit_discarding_others(broker):
        break
    sleep_ms(1)
