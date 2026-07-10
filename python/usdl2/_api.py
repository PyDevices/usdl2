# SPDX-License-Identifier: MIT
"""ctypes bindings for the pydisplay usdl2 API surface."""

from __future__ import annotations

import ctypes
import os
import struct
import threading
from ctypes import CFUNCTYPE, POINTER, c_char_p, c_int, c_uint, c_uint8, c_void_p
from typing import Any

from ._event import EVENT_SIZE, Event
from ._lib import sdl

_TIMER_MAX = 8
_timer_lock = threading.Lock()
_timers: dict[int, dict[str, Any]] = {}
_next_timer_slot = 0


class _Ptr:
    """Opaque SDL handle returned to Python."""

    __slots__ = ("value",)

    def __init__(self, value: int):
        self.value = int(value)

    def __int__(self) -> int:
        return self.value

    def __bool__(self) -> bool:
        return bool(self.value)

    @property
    def _as_parameter_(self):
        return c_void_p(self.value)


class _TimerCallback:
    __slots__ = ("callback",)

    def __init__(self, callback):
        if not callable(callback):
            raise TypeError("callback must be callable")
        self.callback = callback


def _ptr(value) -> _Ptr:
    if isinstance(value, _Ptr):
        return value
    if value is None:
        return _Ptr(0)
    return _Ptr(int(value))


def _none_ptr(obj) -> bool:
    return obj is None or (isinstance(obj, _Ptr) and not obj.value)


def SDL_DEFINE_PIXELFORMAT(type_, order, layout, bits, bytes_):
    return (1 << 28) | (type_ << 24) | (order << 20) | (layout << 16) | (bits << 8) | bytes_


def SDL_Rect(x=0, y=0, w=0, h=0):
    return struct.pack("<iiii", int(x), int(y), int(w), int(h))


def SDL_Point(x=0, y=0):
    return struct.pack("<ii", int(x), int(y))


def SDL_Event(arg=None):
    if arg is None:
        return Event()
    if isinstance(arg, Event):
        return arg
    return Event(arg)


def process_exit(code: int = 0) -> None:
    os._exit(int(code))


def pump_scheduler(max_pending=None) -> int:
  # CPython has no MicroPython scheduler; timers run on SDL threads directly.
    return 0


_timer_trampoline_ref = None


def _configure_sdl(lib):
    lib.SDL_Init.argtypes = [c_uint]
    lib.SDL_Init.restype = c_int
    lib.SDL_InitSubSystem.argtypes = [c_uint]
    lib.SDL_InitSubSystem.restype = c_int
    lib.SDL_Quit.argtypes = []
    lib.SDL_Quit.restype = None
    lib.SDL_GetError.argtypes = []
    lib.SDL_GetError.restype = c_char_p

    lib.SDL_CreateWindow.argtypes = [c_char_p, c_int, c_int, c_int, c_int, c_uint]
    lib.SDL_CreateWindow.restype = c_void_p
    lib.SDL_DestroyWindow.argtypes = [c_void_p]
    lib.SDL_DestroyWindow.restype = None
    lib.SDL_SetWindowSize.argtypes = [c_void_p, c_int, c_int]
    lib.SDL_SetWindowSize.restype = c_int

    lib.SDL_CreateRenderer.argtypes = [c_void_p, c_int, c_uint]
    lib.SDL_CreateRenderer.restype = c_void_p
    lib.SDL_DestroyRenderer.argtypes = [c_void_p]
    lib.SDL_DestroyRenderer.restype = None
    lib.SDL_SetRenderDrawColor.argtypes = [c_void_p, c_uint8, c_uint8, c_uint8, c_uint8]
    lib.SDL_SetRenderDrawColor.restype = c_int
    lib.SDL_SetRenderTarget.argtypes = [c_void_p, c_void_p]
    lib.SDL_SetRenderTarget.restype = c_int
    lib.SDL_RenderClear.argtypes = [c_void_p]
    lib.SDL_RenderClear.restype = c_int
    lib.SDL_RenderPresent.argtypes = [c_void_p]
    lib.SDL_RenderPresent.restype = None
    lib.SDL_RenderSetLogicalSize.argtypes = [c_void_p, c_int, c_int]
    lib.SDL_RenderSetLogicalSize.restype = c_int

    lib.SDL_CreateTexture.argtypes = [c_void_p, c_uint, c_int, c_int, c_int]
    lib.SDL_CreateTexture.restype = c_void_p
    lib.SDL_DestroyTexture.argtypes = [c_void_p]
    lib.SDL_DestroyTexture.restype = None
    lib.SDL_SetTextureBlendMode.argtypes = [c_void_p, c_int]
    lib.SDL_SetTextureBlendMode.restype = c_int
    lib.SDL_UpdateTexture.argtypes = [c_void_p, c_void_p, c_void_p, c_int]
    lib.SDL_UpdateTexture.restype = c_int

    lib.SDL_PollEvent.argtypes = [c_void_p]
    lib.SDL_PollEvent.restype = c_int
    lib.SDL_GetKeyName.argtypes = [c_int]
    lib.SDL_GetKeyName.restype = c_char_p

    lib.SDL_NumJoysticks.argtypes = []
    lib.SDL_NumJoysticks.restype = c_int
    lib.SDL_JoystickOpen.argtypes = [c_int]
    lib.SDL_JoystickOpen.restype = c_void_p
    lib.SDL_JoystickClose.argtypes = [c_void_p]
    lib.SDL_JoystickClose.restype = None
    lib.SDL_JoystickInstanceID.argtypes = [c_void_p]
    lib.SDL_JoystickInstanceID.restype = c_int

    global _timer_trampoline_ref
    _TimerTrampoline = CFUNCTYPE(c_uint, c_uint, c_void_p)

    def _timer_trampoline(interval, param):
        slot = int(param) if param is not None else 0
        with _timer_lock:
            entry = _timers.get(slot)
        if entry is None:
            return 0
        try:
            entry["callback"](interval, entry.get("user_param"))
        except Exception:
            pass
        return entry.get("interval", interval)

    _timer_trampoline_ref = _TimerTrampoline(_timer_trampoline)
    lib.SDL_AddTimer.argtypes = [c_uint, _TimerTrampoline, c_void_p]
    lib.SDL_AddTimer.restype = c_uint
    lib.SDL_RemoveTimer.argtypes = [c_uint]
    lib.SDL_RemoveTimer.restype = c_int

    class SDL_Rect(ctypes.Structure):
        _fields_ = [("x", c_int), ("y", c_int), ("w", c_int), ("h", c_int)]

    lib._SDL_Rect = SDL_Rect
    lib.SDL_RenderFillRect.argtypes = [c_void_p, POINTER(SDL_Rect)]
    lib.SDL_RenderFillRect.restype = c_int
    lib.SDL_RenderCopy.argtypes = [c_void_p, c_void_p, POINTER(SDL_Rect), POINTER(SDL_Rect)]
    lib.SDL_RenderCopy.restype = c_int

    class SDL_DisplayMode(ctypes.Structure):
        _fields_ = [
            ("format", c_uint),
            ("w", c_int),
            ("h", c_int),
            ("refresh_rate", c_int),
            ("driverdata", c_void_p),
        ]

    lib._SDL_DisplayMode = SDL_DisplayMode
    lib.SDL_GetDisplayUsableBounds.argtypes = [c_int, POINTER(SDL_Rect)]
    lib.SDL_GetDisplayUsableBounds.restype = c_int
    lib.SDL_GetDesktopDisplayMode.argtypes = [c_int, POINTER(SDL_DisplayMode)]
    lib.SDL_GetDesktopDisplayMode.restype = c_int

    class SDL_Point(ctypes.Structure):
        _fields_ = [("x", c_int), ("y", c_int)]

    lib._SDL_Point = SDL_Point
    lib.SDL_RenderCopyEx.argtypes = [
        c_void_p,
        c_void_p,
        POINTER(SDL_Rect),
        POINTER(SDL_Rect),
        ctypes.c_double,
        POINTER(SDL_Point),
        c_int,
    ]
    lib.SDL_RenderCopyEx.restype = c_int
    return lib


_lib = None


def _get_lib():
    global _lib
    if _lib is None:
        _lib = _configure_sdl(sdl())
    return _lib


def _rect_from_bytes(obj) -> Any:
    if obj is None:
        return None
    data = bytes(obj)
    if len(data) < 16:
        raise ValueError("rect buffer too small")
    x, y, w, h = struct.unpack("<iiii", data[:16])
    rect = _get_lib()._SDL_Rect(x, y, w, h)
    return rect


def _title_bytes(title) -> bytes:
    if isinstance(title, str):
        return title.encode()
    if isinstance(title, (bytes, bytearray)):
        return bytes(title)
    return bytes(memoryview(title))


def SDL_Init(flags: int) -> int:
    return _get_lib().SDL_Init(int(flags))


def SDL_InitSubSystem(flags: int) -> int:
    return _get_lib().SDL_InitSubSystem(int(flags))


def SDL_Quit() -> None:
    _get_lib().SDL_Quit()


def SDL_GetError() -> str:
    err = _get_lib().SDL_GetError()
    if not err:
        return ""
    return err.decode("utf-8", "replace")


def SDL_CreateWindow(title, x, y, w, h, window_flags) -> _Ptr:
    win = _get_lib().SDL_CreateWindow(_title_bytes(title), int(x), int(y), int(w), int(h), int(window_flags))
    return _ptr(win)


def SDL_DestroyWindow(window) -> None:
    _get_lib().SDL_DestroyWindow(c_void_p(_ptr(window).value))


def SDL_SetWindowSize(window, w, h) -> None:
    _get_lib().SDL_SetWindowSize(c_void_p(_ptr(window).value), int(w), int(h))


def SDL_CreateRenderer(window, index, flags) -> _Ptr:
    ren = _get_lib().SDL_CreateRenderer(c_void_p(_ptr(window).value), int(index), int(flags))
    return _ptr(ren)


def SDL_DestroyRenderer(renderer) -> None:
    _get_lib().SDL_DestroyRenderer(c_void_p(_ptr(renderer).value))


def SDL_SetRenderDrawColor(renderer, r, g, b, a) -> int:
    return _get_lib().SDL_SetRenderDrawColor(
        c_void_p(_ptr(renderer).value), int(r), int(g), int(b), int(a)
    )


def SDL_SetRenderTarget(renderer, texture) -> int:
    tex = None if _none_ptr(texture) else c_void_p(_ptr(texture).value)
    return _get_lib().SDL_SetRenderTarget(c_void_p(_ptr(renderer).value), tex)


def SDL_RenderClear(renderer) -> None:
    _get_lib().SDL_RenderClear(c_void_p(_ptr(renderer).value))


def SDL_RenderCopy(renderer, texture, src, dst) -> None:
    lib = _get_lib()
    src_ptr = ctypes.byref(_rect_from_bytes(src)) if src is not None else None
    dst_ptr = ctypes.byref(_rect_from_bytes(dst)) if dst is not None else None
    lib.SDL_RenderCopy(
        c_void_p(_ptr(renderer).value),
        c_void_p(_ptr(texture).value),
        src_ptr,
        dst_ptr,
    )


def SDL_RenderCopyEx(renderer, texture, src, dst, angle, center, flip) -> None:
    lib = _get_lib()
    src_ptr = ctypes.byref(_rect_from_bytes(src)) if src is not None else None
    dst_ptr = ctypes.byref(_rect_from_bytes(dst)) if dst is not None else None
    center_pt = None
    if center is not None:
        cx, cy = struct.unpack("<ii", bytes(center)[:8])
        center_pt = lib._SDL_Point(cx, cy)
    lib.SDL_RenderCopyEx(
        c_void_p(_ptr(renderer).value),
        c_void_p(_ptr(texture).value),
        src_ptr,
        dst_ptr,
        float(angle),
        center_pt,
        int(flip),
    )


def SDL_RenderPresent(renderer) -> None:
    _get_lib().SDL_RenderPresent(c_void_p(_ptr(renderer).value))


def SDL_RenderFillRect(renderer, rect_bytes) -> int:
    rect = _rect_from_bytes(rect_bytes)
    return _get_lib().SDL_RenderFillRect(c_void_p(_ptr(renderer).value), rect)


def SDL_RenderSetLogicalSize(renderer, w, h) -> int:
    return _get_lib().SDL_RenderSetLogicalSize(c_void_p(_ptr(renderer).value), int(w), int(h))


def SDL_CreateTexture(renderer, pixel_format, access, w, h) -> _Ptr:
    tex = _get_lib().SDL_CreateTexture(
        c_void_p(_ptr(renderer).value), int(pixel_format), int(access), int(w), int(h)
    )
    return _ptr(tex)


def SDL_DestroyTexture(texture) -> None:
    _get_lib().SDL_DestroyTexture(c_void_p(_ptr(texture).value))


def SDL_SetTextureBlendMode(texture, mode) -> int:
    return _get_lib().SDL_SetTextureBlendMode(c_void_p(_ptr(texture).value), int(mode))


def SDL_UpdateTexture(texture, rect_bytes, pixels, pitch) -> int:
    lib = _get_lib()
    rect_ptr = ctypes.byref(_rect_from_bytes(rect_bytes)) if rect_bytes is not None else None
    if isinstance(pixels, int):
        buf = c_void_p(pixels)
    else:
        mv = memoryview(pixels)
        buf = (c_uint8 * len(mv)).from_buffer(mv)
    return lib.SDL_UpdateTexture(
        c_void_p(_ptr(texture).value),
        rect_ptr,
        buf,
        int(pitch),
    )


def SDL_PollEvent(event) -> bool:
    if not isinstance(event, Event):
        raise TypeError("SDL_PollEvent expects usdl2.Event")
    raw = (c_uint8 * EVENT_SIZE)()
    rc = _get_lib().SDL_PollEvent(raw)
    if rc:
        event._data[:] = bytes(raw)
    return bool(rc)


def SDL_GetKeyName(sym) -> str:
    name = _get_lib().SDL_GetKeyName(int(sym))
    if not name:
        return ""
    return name.decode("utf-8", "replace")


def SDL_NumJoysticks() -> int:
    return _get_lib().SDL_NumJoysticks()


def SDL_JoystickOpen(index) -> _Ptr:
    return _ptr(_get_lib().SDL_JoystickOpen(int(index)))


def SDL_JoystickClose(joystick) -> None:
    _get_lib().SDL_JoystickClose(c_void_p(_ptr(joystick).value))


def SDL_JoystickInstanceID(joystick) -> int:
    return _get_lib().SDL_JoystickInstanceID(c_void_p(_ptr(joystick).value))


def SDL_GetDisplayUsableBounds(display_index, rect) -> int:
    lib = _get_lib()
    if isinstance(rect, (bytes, bytearray, memoryview)):
        buf = bytearray(rect) if not isinstance(rect, bytearray) else rect
        if len(buf) < 16:
            raise ValueError("rect buffer too small")
        r = lib._SDL_Rect()
        rc = lib.SDL_GetDisplayUsableBounds(int(display_index), ctypes.byref(r))
        if rc == 0:
            struct.pack_into("<iiii", buf, 0, r.x, r.y, r.w, r.h)
        return rc
    r = rect if hasattr(rect, "_fields_") else _rect_from_bytes(rect)
    return lib.SDL_GetDisplayUsableBounds(int(display_index), ctypes.byref(r))


def SDL_GetDesktopDisplayMode(display_index, mode) -> int:
    lib = _get_lib()
    if isinstance(mode, (bytes, bytearray, memoryview)):
        buf = bytearray(mode) if not isinstance(mode, bytearray) else mode
        if len(buf) < ctypes.sizeof(lib._SDL_DisplayMode):
            raise ValueError("display mode buffer too small")
        m = lib._SDL_DisplayMode()
        rc = lib.SDL_GetDesktopDisplayMode(int(display_index), ctypes.byref(m))
        if rc == 0:
            struct.pack_into(
                "<Iiii",
                buf,
                0,
                m.format,
                m.w,
                m.h,
                m.refresh_rate,
            )
        return rc
    m = mode if hasattr(mode, "_fields_") else lib._SDL_DisplayMode()
    return lib.SDL_GetDesktopDisplayMode(int(display_index), ctypes.byref(m))


def SDL_TimerCallback(callback) -> _TimerCallback:
    return _TimerCallback(callback)


def SDL_AddTimer(interval, callback_obj, user_param) -> _Ptr:
    if not isinstance(callback_obj, _TimerCallback):
        raise TypeError("callback must be from SDL_TimerCallback()")
    global _next_timer_slot
    lib = _get_lib()
    if _timer_trampoline_ref is None:
        raise RuntimeError("SDL timer trampoline not configured")
    with _timer_lock:
        active = sum(1 for e in _timers.values() if isinstance(e, dict))
        if active >= _TIMER_MAX:
            raise RuntimeError("too many SDL timers")
        slot = _next_timer_slot
        _next_timer_slot += 1
        entry = {
            "callback": callback_obj.callback,
            "user_param": user_param,
            "interval": int(interval),
        }
        timer_id = lib.SDL_AddTimer(int(interval), _timer_trampoline_ref, c_void_p(slot))
        if not timer_id:
            raise RuntimeError(SDL_GetError() or "SDL_AddTimer failed")
        entry["id"] = timer_id
        _timers[slot] = entry
        return _ptr(timer_id)


def SDL_RemoveTimer(timer) -> int:
    timer_id = _ptr(timer).value
    with _timer_lock:
        entry = None
        for slot, e in list(_timers.items()):
            if e.get("id") == timer_id:
                entry = e
                del _timers[slot]
                break
    if entry is None:
        return 0
    removed = _get_lib().SDL_RemoveTimer(timer_id)
    return 1 if removed else 0
