# SPDX-License-Identifier: MIT
"""python-for-android recipe: native usdl2 C extension (links against p4a SDL2)."""

import os
from os.path import join

from pythonforandroid.recipe import CompiledComponentsPythonRecipe


class Usdl2Recipe(CompiledComponentsPythonRecipe):
    version = "main"
    url = "https://github.com/PyDevices/usdl2/archive/{version}.zip"
    name = "usdl2"
    depends = ["sdl2", "setuptools"]
    call_hostpython_via_targetpython = False
    install_in_hostpython = False
    site_packages_name = "usdl2"

    def download_if_necessary(self):
        if os.environ.get("P4A_usdl2_DIR"):
            return
        super().download_if_necessary()

    def get_build_dir(self, arch):
        local = os.environ.get("P4A_usdl2_DIR")
        if local:
            return local
        return super().get_build_dir(arch)

    def get_recipe_env(self, arch, **kwargs):
        env = super().get_recipe_env(arch, **kwargs)
        # p4a SDL2 bootstrap headers (same pattern as Kivy).
        sdl_include = join(self.ctx.bootstrap.build_dir, "jni", "SDL", "include")
        env["CFLAGS"] = env.get("CFLAGS", "") + f" -I{sdl_include}"
        env["CPPFLAGS"] = env.get("CPPFLAGS", "") + f" -I{sdl_include}"
        # setup.py uses pkg-config on non-win32; provide flags for the NDK cross build.
        env["PKG_CONFIG_PATH"] = env.get("PKG_CONFIG_PATH", "")
        # Force include path even if pkg-config is absent in the NDK env.
        env["USDL2_ANDROID"] = "1"
        env["USDL2_SDL2_INCLUDE"] = sdl_include
        return env


recipe = Usdl2Recipe()
