# SPDX-License-Identifier: MIT
"""Android / desktop SDL board config for the pydisplay demo APK."""

import sys

import eventsys
import usdl2
from displaysys.sdldisplay import SDLDisplay, get_events

# Landscape logical framebuffer (matches buildozer orientation).
width = 480
height = 320
rotation = 0
scale = 1.0

if sys.platform == "android":
    title = "pydisplay on Android"
    window_flags = usdl2.SDL_WINDOW_FULLSCREEN_DESKTOP | usdl2.SDL_WINDOW_ALLOW_HIGHDPI
else:
    title = f"pydisplay ({sys.platform})"
    window_flags = usdl2.SDL_WINDOW_SHOWN | usdl2.SDL_WINDOW_ALLOW_HIGHDPI
    scale = 1.5

display_drv = SDLDisplay(
    width=width,
    height=height,
    rotation=rotation,
    title=title,
    scale=scale,
    window_flags=window_flags,
)

broker = eventsys.Broker()

events_dev = broker.create(
    type=eventsys.QUEUE,
    read=get_events,
    data=display_drv,
)

broker.register_quit_cleanup(display_drv)

display_drv.fill(0)
