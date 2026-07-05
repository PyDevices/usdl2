# SPDX-License-Identifier: MIT
"""
CPython ctypes implementation of the pydisplay usdl2 API.

Used on Android (python-for-android SDL2 bootstrap) and as a desktop fallback
when the native MicroPython module is unavailable.
"""

_USE_FFI = True

from ._api import *  # noqa: F403
from ._constants import *  # noqa: F403
from ._event import Event

__all__ = ["Event", "_USE_FFI"]
