[app]
title = pydisplay Android Demo
package.name = pydisplaydemo
package.domain = org.pydevices
source.dir = .
source.include_exts = py
version = 0.2.0
requirements = python3,sdl2,usdl2,pydisplay
orientation = landscape
fullscreen = 0
android.api = 31
android.minapi = 24
android.archs = arm64-v8a, armeabi-v7a
android.bootstrap = sdl2
android.permissions = INTERNET

# Local recipes: usdl2 (ctypes SDL) + pydisplay (src/lib packages).
p4a.local_recipes = ../p4a_recipes

[buildozer]
log_level = 2
warn_on_root = 0
