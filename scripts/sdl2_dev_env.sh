# Source or copy these before Windows usdl2 builds (do not vendor SDL2 in-repo).
#
# Layout relative to this file: usdl2/scripts → …/gh/other (alongside emsdk).
_OTHER_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../../other" && pwd)"
#
# MinGW (micropython.exe) — unpack SDL2-devel-*-mingw.zip:
export SDL2_DEV_MINGW="${SDL2_DEV_MINGW:-$_OTHER_DIR/SDL2-2.30.10}"
#
# MSVC (python.exe) — unpack SDL2-devel-*-VC.zip onto the Windows drive for WSL→Windows builds:
export SDL2_DEV_MSVC_WIN="${SDL2_DEV_MSVC_WIN:-C:\\SDL2-2.30.10-VC}"
export SDL2_DEV_MSVC="${SDL2_DEV_MSVC:-/mnt/c/SDL2-2.30.10-VC}"
#
# Examples:
#   export SDL2_DEV="$SDL2_DEV_MINGW"
#   ./build_mp.sh --port windows --variant standard
#
#   cmd.exe /c "set SDL2_DEV=$SDL2_DEV_MSVC_WIN&& pip.exe install -e $(wslpath -w "$PWD")"
#   cmd.exe /c "set PATH=C:\\SDL2-2.30.10-VC\\lib\\x64;%PATH%&& set SDL_VIDEODRIVER=dummy&& python.exe test_usdl2.py"
