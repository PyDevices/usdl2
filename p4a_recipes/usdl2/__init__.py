# SPDX-License-Identifier: MIT
"""python-for-android recipe: CPython ctypes usdl2 (links against p4a SDL2 at runtime)."""

import os

from pythonforandroid.recipe import PythonRecipe


class Usdl2Recipe(PythonRecipe):
    version = "main"
    url = "https://github.com/PyDevices/usdl2/archive/{version}.zip"
    name = "usdl2"
    depends = ["sdl2"]
    call_hostpython_via_targetpython = False
    install_in_hostpython = False

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
        env["USDL2_FFI"] = "1"
        return env


recipe = Usdl2Recipe()
