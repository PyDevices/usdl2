[app]
title = pydisplay LVGL Android Demo
package.name = pydisplaydemo
package.domain = org.pydevices
source.dir = .
source.include_exts = py
source.main = main_lvgl.py
version = 0.3.0
requirements = python3==3.13,sdl2,usdl2,pydisplay,lvglcpython
orientation = landscape
fullscreen = 0
android.api = 31
android.minapi = 24
android.archs = arm64-v8a, armeabi-v7a
android.bootstrap = sdl2
android.permissions = INTERNET

# Prebuilt lvgl-cpython wheels (android_21_arm64_v8a, etc.) from TestPyPI when available.
p4a.extra_index_url = https://test.pypi.org/simple/,https://pypi.org/simple/

# Local recipes: usdl2, pydisplay, lvglcpython.
p4a.local_recipes = ../p4a_recipes

[buildozer]
log_level = 2
warn_on_root = 0
