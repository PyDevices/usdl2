[app]
title = usdl2 Android Demo
package.name = usdl2demo
package.domain = org.pydevices
source.dir = .
source.include_exts = py
version = 0.1.0
requirements = python3,sdl2,usdl2
orientation = landscape
fullscreen = 0
android.api = 31
android.minapi = 24
android.archs = arm64-v8a, armeabi-v7a
android.bootstrap = sdl2
android.permissions = INTERNET

# Point at the usdl2 repo recipe (relative to this spec file).
p4a.local_recipes = ../p4a_recipes

[buildozer]
log_level = 2
warn_on_root = 0
