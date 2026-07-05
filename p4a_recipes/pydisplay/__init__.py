# SPDX-License-Identifier: MIT
"""python-for-android recipe: pydisplay (SDLDisplay stack for Android)."""

import glob
import os
import shutil
from os.path import dirname, isfile, join

from pythonforandroid.recipe import PythonRecipe


class PydisplayRecipe(PythonRecipe):
    version = "main"
    url = "https://github.com/PyDevices/pydisplay/archive/{version}.zip"
    name = "pydisplay"
    depends = ["usdl2"]
    call_hostpython_via_targetpython = False
    install_in_hostpython = False

    def download_if_necessary(self):
        if os.environ.get("P4A_pydisplay_DIR"):
            return
        super().download_if_necessary()

    def get_build_dir(self, arch):
        local = os.environ.get("P4A_pydisplay_DIR")
        if local:
            return local
        return super().get_build_dir(arch)

    def prebuild_arch(self, arch):
        super().prebuild_arch(arch)
        build_dir = self.get_build_dir(arch.arch)
        setup_src = join(dirname(__file__), "setup.py")
        shutil.copy2(setup_src, join(build_dir, "setup.py"))

    def build_arch(self, arch):
        super().build_arch(arch)
        build_dir = self.get_build_dir(arch.arch)
        addons = join(build_dir, "src", "add_ons")
        python_install = self.ctx.get_python_install_dir(arch.arch)
        site_dirs = glob.glob(join(python_install, "lib", "python*", "site-packages"))
        if not site_dirs:
            return
        site = site_dirs[0]
        for name in ("display_driver.py", "lv_utils.py"):
            src = join(addons, name)
            if isfile(src):
                shutil.copy2(src, join(site, name))


recipe = PydisplayRecipe()
