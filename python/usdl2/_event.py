# SPDX-License-Identifier: MIT
"""SDL_Event wrapper matching native usdl2 event layout."""

from __future__ import annotations

import struct

EVENT_SIZE = 56

_JOY_MIN = 1536
_JOY_MAX = 1542


class _SubView:
    __slots__ = ("_buf", "_base")

    def __init__(self, buf: bytearray, base: int):
        self._buf = buf
        self._base = base

    def _u32(self, off: int) -> int:
        return struct.unpack_from("<I", self._buf, self._base + off)[0]

    def _i32(self, off: int) -> int:
        return struct.unpack_from("<i", self._buf, self._base + off)[0]

    def _i16(self, off: int) -> int:
        return struct.unpack_from("<h", self._buf, self._base + off)[0]

    @property
    def windowID(self) -> int:
        return self._u32(0)

    @property
    def which(self) -> int:
        etype = struct.unpack_from("<I", self._buf, 0)[0]
        if _JOY_MIN <= etype <= _JOY_MAX:
            return self._i32(0)
        return self._u32(4)

    @property
    def state(self) -> int:
        etype = struct.unpack_from("<I", self._buf, 0)[0]
        if self._base == 8 and etype == 1024:  # SDL_MOUSEMOTION
            return self._u32(8)
        if etype in (1539, 1540):  # joy button
            return self._buf[self._base + 5]
        return self._u32(4)

    @property
    def x(self) -> int:
        return self._i32(12)

    @property
    def y(self) -> int:
        return self._i32(16)

    @property
    def xrel(self) -> int:
        etype = struct.unpack_from("<I", self._buf, 0)[0]
        if etype == 1537:  # SDL_JOYBALLMOTION
            return self._i16(8)
        return self._i32(20)

    @property
    def yrel(self) -> int:
        etype = struct.unpack_from("<I", self._buf, 0)[0]
        if etype == 1537:
            return self._i16(10)
        return self._i32(24)

    @property
    def button(self) -> int:
        etype = struct.unpack_from("<I", self._buf, 0)[0]
        if etype in (1539, 1540):
            return self._buf[self._base + 4]
        return self._buf[self._base + 8]

    @property
    def clicks(self) -> int:
        return self._buf[self._base + 10]

    @property
    def direction(self) -> int:
        return self._u32(16)

    @property
    def preciseX(self) -> float:
        return struct.unpack_from("<f", self._buf, self._base + 0)[0]

    @property
    def preciseY(self) -> float:
        return struct.unpack_from("<f", self._buf, self._base + 4)[0]

    @property
    def sym(self):
        return _KeySym(self._buf, self._base)

    @property
    def axis(self) -> int:
        return self._buf[self._base + 4]

    @property
    def value(self) -> int:
        etype = struct.unpack_from("<I", self._buf, 0)[0]
        if etype == 1536:  # SDL_JOYAXISMOTION
            return self._i16(8)
        if etype == 1538:  # SDL_JOYHATMOTION
            return self._buf[self._base + 5]
        return 0

    @property
    def ball(self) -> int:
        return self._buf[self._base + 4]

    @property
    def hat(self) -> int:
        return self._buf[self._base + 4]


class _KeySym:
    __slots__ = ("_buf", "_base")

    def __init__(self, buf: bytearray, base: int):
        self._buf = buf
        self._base = base

    @property
    def sym(self) -> int:
        return struct.unpack_from("<i", self._buf, self._base + 0)[0]

    @property
    def mod(self) -> int:
        return struct.unpack_from("<H", self._buf, self._base + 4)[0]

    @property
    def scancode(self) -> int:
        return struct.unpack_from("<i", self._buf, self._base + 8)[0]


class Event:
    """56-byte SDL event buffer with pydisplay-compatible subviews."""

    __slots__ = ("_data",)

    def __init__(self, data=None):
        if data is None:
            self._data = bytearray(EVENT_SIZE)
        elif isinstance(data, (bytes, bytearray, memoryview)):
            self._data = bytearray(data[:EVENT_SIZE].ljust(EVENT_SIZE, b"\x00"))
        elif isinstance(data, Event):
            self._data = bytearray(data._data)
        else:
            raise TypeError("Event() expects bytes-like or Event")

    @property
    def type(self) -> int:
        return struct.unpack_from("<I", self._data, 0)[0]

    @type.setter
    def type(self, value: int) -> None:
        struct.pack_into("<I", self._data, 0, int(value))

    @property
    def motion(self) -> _SubView:
        return _SubView(self._data, 0)

    @property
    def button(self) -> _SubView:
        return _SubView(self._data, 0)

    @property
    def wheel(self) -> _SubView:
        return _SubView(self._data, 0)

    @property
    def key(self) -> _SubView:
        return _SubView(self._data, 0)

    @property
    def jaxis(self) -> _SubView:
        return _SubView(self._data, 8)

    @property
    def jball(self) -> _SubView:
        return _SubView(self._data, 8)

    @property
    def jhat(self) -> _SubView:
        return _SubView(self._data, 8)

    @property
    def jbutton(self) -> _SubView:
        return _SubView(self._data, 8)

    def __len__(self) -> int:
        return EVENT_SIZE

    def __bytes__(self) -> bytes:
        return bytes(self._data)

    def __buffer__(self, flags):
        return memoryview(self._data)
