#!/usr/bin/env bash
# Build a modern SDL2 into /opt/sdl2 inside the manylinux container (yum SDL2 is too old).
set -euo pipefail

SDL2_VERSION="${USDL2_SDL2_VERSION:-2.30.11}"
PREFIX="${USDL2_SDL2_LINUX_PREFIX:-/opt/sdl2}"
SRC_ROOT="${USDL2_SDL2_SRC:-/tmp/sdl2-src}"

if [[ -f "$PREFIX/lib/pkgconfig/sdl2.pc" || -f "$PREFIX/lib64/pkgconfig/sdl2.pc" ]]; then
    echo "==> Reusing SDL2 at $PREFIX"
    exit 0
fi

echo "==> Building SDL2 $SDL2_VERSION → $PREFIX"
mkdir -p "$SRC_ROOT"
TARBALL="$SRC_ROOT/SDL2-${SDL2_VERSION}.tar.gz"
if [[ ! -f "$TARBALL" ]]; then
    curl -fsSL -o "$TARBALL" \
        "https://github.com/libsdl-org/SDL/releases/download/release-${SDL2_VERSION}/SDL2-${SDL2_VERSION}.tar.gz"
fi
SDL_SRC="$SRC_ROOT/SDL2-${SDL2_VERSION}"
if [[ ! -d "$SDL_SRC" ]]; then
    tar -xzf "$TARBALL" -C "$SRC_ROOT"
fi

# Minimal deps for a headless/shared SDL2 used only at link/import time in wheels.
if command -v yum >/dev/null 2>&1; then
    yum install -y cmake gcc gcc-c++ make curl tar \
        libX11-devel libXext-devel libXrandr-devel libXcursor-devel \
        libXi-devel libXinerama-devel libXss-devel mesa-libGL-devel \
        alsa-lib-devel pulseaudio-libs-devel || true
elif command -v apt-get >/dev/null 2>&1; then
    apt-get update
    apt-get install -y cmake gcc g++ make curl tar \
        libx11-dev libxext-dev libxrandr-dev libxcursor-dev \
        libxi-dev libxinerama-dev libxss-dev libgl1-mesa-dev \
        libasound2-dev libpulse-dev || true
fi

BUILD_DIR="$SRC_ROOT/build-linux"
rm -rf "$BUILD_DIR"
cmake -S "$SDL_SRC" -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$PREFIX" \
    -DSDL_SHARED=ON \
    -DSDL_STATIC=OFF \
    -DSDL_TEST=OFF \
    -DSDL_TESTS=OFF
cmake --build "$BUILD_DIR" --parallel "$(nproc 2>/dev/null || echo 2)"
cmake --install "$BUILD_DIR"
echo "==> SDL2 installed; PKG_CONFIG_PATH should include $PREFIX/lib/pkgconfig"
