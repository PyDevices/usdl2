#!/usr/bin/env bash
# Build SDL2 shared libs + headers for Android ABIs used by cibuildwheel wheels.
# Output: .sdl2-android/{arm64_v8a,x86_64}/include/SDL2 and …/lib/libSDL2.so
#
# Requires: cmake, ANDROID_HOME (or ANDROID_SDK_ROOT), NDK under SDK or ANDROID_NDK_HOME.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

SDL2_VERSION="${USDL2_SDL2_VERSION:-2.30.11}"
API_LEVEL="${ANDROID_API_LEVEL:-21}"
OUT_ROOT="${USDL2_SDL2_ANDROID_ROOT:-$ROOT/.sdl2-android}"
SRC_ROOT="${USDL2_SDL2_SRC:-$ROOT/.sdl2-src}"

ANDROID_HOME="${ANDROID_HOME:-${ANDROID_SDK_ROOT:-}}"
if [[ -z "$ANDROID_HOME" ]]; then
    echo "ANDROID_HOME / ANDROID_SDK_ROOT must be set" >&2
    exit 1
fi

if [[ -n "${ANDROID_NDK_HOME:-}" && -d "$ANDROID_NDK_HOME" ]]; then
    NDK="$ANDROID_NDK_HOME"
elif [[ -d "$ANDROID_HOME/ndk" ]]; then
    NDK="$(find "$ANDROID_HOME/ndk" -mindepth 1 -maxdepth 1 -type d 2>/dev/null | sort -V | tail -1)"
else
    # buildozer layout
    NDK="$(find "$HOME/.buildozer/android/platform" -maxdepth 1 -type d -name 'android-ndk-*' 2>/dev/null | sort -V | tail -1)"
fi
if [[ -z "${NDK:-}" || ! -d "$NDK" ]]; then
    echo "Android NDK not found (set ANDROID_NDK_HOME)" >&2
    exit 1
fi

TOOLCHAIN="$NDK/build/cmake/android.toolchain.cmake"
if [[ ! -f "$TOOLCHAIN" ]]; then
    echo "Missing Android CMake toolchain: $TOOLCHAIN" >&2
    exit 1
fi

echo "==> SDL2 $SDL2_VERSION for Android API $API_LEVEL"
echo "    NDK=$NDK"
echo "    OUT=$OUT_ROOT"

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

build_abi() {
    local ndk_abi="$1"   # arm64-v8a | x86_64
    local wheel_abi="$2" # arm64_v8a | x86_64
    local prefix="$OUT_ROOT/$wheel_abi"
    local build_dir="$SRC_ROOT/build-$wheel_abi"

    if [[ -f "$prefix/lib/libSDL2.so" && -f "$prefix/include/SDL2/SDL.h" ]]; then
        echo "==> Reusing $prefix"
        return 0
    fi

    echo "==> Building SDL2 for $ndk_abi → $prefix"
    rm -rf "$build_dir"
    mkdir -p "$build_dir"
    cmake -S "$SDL_SRC" -B "$build_dir" \
        -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN" \
        -DANDROID_ABI="$ndk_abi" \
        -DANDROID_PLATFORM="android-${API_LEVEL}" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="$prefix" \
        -DSDL_SHARED=ON \
        -DSDL_STATIC=OFF \
        -DSDL_TEST=OFF \
        -DSDL_TESTS=OFF \
        -DSDL_INSTALL_TESTS=OFF
    cmake --build "$build_dir" --parallel "$(nproc 2>/dev/null || echo 2)"
    cmake --install "$build_dir"

    test -f "$prefix/lib/libSDL2.so" || test -f "$prefix/lib64/libSDL2.so"
    # Normalize lib dir
    if [[ ! -f "$prefix/lib/libSDL2.so" && -f "$prefix/lib64/libSDL2.so" ]]; then
        mkdir -p "$prefix/lib"
        cp -a "$prefix/lib64/libSDL2.so"* "$prefix/lib/"
    fi
    test -f "$prefix/include/SDL2/SDL.h"
}

build_abi arm64-v8a arm64_v8a
build_abi x86_64 x86_64

echo "==> SDL2 Android prefixes ready under $OUT_ROOT"
