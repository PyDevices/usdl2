# SPDX-License-Identifier: MIT
"""python-for-android recipe: lvgl-cpython (prebuilt Android wheel or source build)."""

import os

from pythonforandroid.recipe import PyProjectRecipe


class LvglCpythonRecipe(PyProjectRecipe):
    version = "main"
    url = "https://github.com/PyDevices/lv_cpython_mod/archive/{version}.zip"
    name = "lvglcpython"
    depends = []
    call_hostpython_via_targetpython = False

    def get_pip_name(self):
        return "lvgl-cpython"

    def download_if_necessary(self):
        if os.environ.get("P4A_lvgl_cpython_DIR"):
            return
        super().download_if_necessary()

    def get_build_dir(self, arch):
        local = os.environ.get("P4A_lvgl_cpython_DIR")
        if local:
            return local
        return super().get_build_dir(arch)


recipe = LvglCpythonRecipe()
