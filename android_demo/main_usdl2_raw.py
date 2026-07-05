# SPDX-License-Identifier: MIT
"""Minimal SDL2 touch demo (raw usdl2, no pydisplay). Kept as a reference."""

import time

import usdl2

WIDTH, HEIGHT = 480, 320
COLOR_BG = 0x001F
COLOR_DOT = 0xF800


def main():
    assert usdl2._USE_FFI, "Android build expects ctypes usdl2"
    usdl2.SDL_Init(usdl2.SDL_INIT_VIDEO)
    win = usdl2.SDL_CreateWindow(
        b"usdl2 raw",
        usdl2.SDL_WINDOWPOS_CENTERED,
        usdl2.SDL_WINDOWPOS_CENTERED,
        WIDTH,
        HEIGHT,
        usdl2.SDL_WINDOW_SHOWN | usdl2.SDL_WINDOW_ALLOW_HIGHDPI,
    )
    if not win:
        raise SystemExit(usdl2.SDL_GetError())
    renderer = usdl2.SDL_CreateRenderer(
        win, -1, usdl2.SDL_RENDERER_ACCELERATED | usdl2.SDL_RENDERER_PRESENTVSYNC
    )
    if not renderer:
        raise SystemExit(usdl2.SDL_GetError())

    texture = usdl2.SDL_CreateTexture(
        renderer,
        usdl2.SDL_PIXELFORMAT_RGB565,
        usdl2.SDL_TEXTUREACCESS_STREAMING,
        WIDTH,
        HEIGHT,
    )
    pitch = WIDTH * 2
    framebuffer = bytearray(WIDTH * HEIGHT * 2)

    def fill(color):
        lo = color & 0xFF
        hi = (color >> 8) & 0xFF
        for i in range(0, len(framebuffer), 2):
            framebuffer[i] = lo
            framebuffer[i + 1] = hi

    def dot(x, y, color, radius=8):
        lo = color & 0xFF
        hi = (color >> 8) & 0xFF
        for dy in range(-radius, radius + 1):
            for dx in range(-radius, radius + 1):
                px, py = x + dx, y + dy
                if 0 <= px < WIDTH and 0 <= py < HEIGHT:
                    off = (py * WIDTH + px) * 2
                    framebuffer[off] = lo
                    framebuffer[off + 1] = hi

    fill(COLOR_BG)
    event = usdl2.SDL_Event()
    running = True
    while running:
        while usdl2.SDL_PollEvent(event):
            if event.type == usdl2.SDL_QUIT:
                running = False
            elif event.type == usdl2.SDL_MOUSEBUTTONDOWN:
                dot(event.button.x, event.button.y, COLOR_DOT)
            elif event.type == usdl2.SDL_MOUSEMOTION:
                if event.motion.state & usdl2.SDL_BUTTON_LMASK:
                    dot(event.motion.x, event.motion.y, COLOR_DOT)

        rect = usdl2.SDL_Rect(0, 0, WIDTH, HEIGHT)
        usdl2.SDL_UpdateTexture(texture, rect, framebuffer, pitch)
        usdl2.SDL_RenderCopy(renderer, texture, None, None)
        usdl2.SDL_RenderPresent(renderer)
        time.sleep(0.001)

    usdl2.SDL_DestroyTexture(texture)
    usdl2.SDL_DestroyRenderer(renderer)
    usdl2.SDL_DestroyWindow(win)
    usdl2.SDL_Quit()


if __name__ == "__main__":
    main()
