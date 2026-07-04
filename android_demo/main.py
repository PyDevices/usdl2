# SPDX-License-Identifier: MIT
"""
pydisplay Android demo — SDLDisplay + eventsys + multimer.

p4a SDL2 bootstrap runs this file as the app entry point.
"""

import multimer
from board_config import broker, display_drv
from displaysys import color565
from eventsys import events

BG = color565(0, 0, 64)
DOT = color565(255, 128, 0)
RADIUS = 10


def _draw_dot(x, y):
    x, y = display_drv.translate_point((x, y))
    display_drv.fill_rect(x - RADIUS, y - RADIUS, RADIUS * 2, RADIUS * 2, DOT)
    display_drv.show()


def main():
    display_drv.fill(BG)
    display_drv.show()
    running = True
    while running:
        for event in broker.poll():
            if event.type == events.QUIT:
                running = False
            elif event.type == events.MOUSEBUTTONDOWN:
                _draw_dot(*event.pos)
            elif event.type == events.MOUSEMOTION and event.buttons[0]:
                _draw_dot(*event.pos)
        multimer.pump()


if __name__ == "__main__":
    main()
