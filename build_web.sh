#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${ROOT_DIR}/build/web"

if [[ -z "${QT_WASM_PREFIX:-}" ]]; then
    echo "QT_WASM_PREFIX is required. Set it to the Qt for WebAssembly prefix path." >&2
    exit 1
fi

if [[ -z "${EMSCRIPTEN:-}" && -n "${EMSDK:-}" ]]; then
    EMSCRIPTEN="${EMSDK}/upstream/emscripten"
fi

configure_with_qt_cmake() {
    qt-cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" \
        -DSEICHE_BUILD_WEB=ON \
        -DCMAKE_PREFIX_PATH="${QT_WASM_PREFIX}" \
        -DCMAKE_BUILD_TYPE=Release
}

configure_with_cmake() {
    if [[ -z "${EMSCRIPTEN:-}" ]]; then
        echo "EMSCRIPTEN or EMSDK is required when qt-cmake is not available." >&2
        exit 1
    fi

    cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -G Ninja \
        -DSEICHE_BUILD_WEB=ON \
        -DCMAKE_TOOLCHAIN_FILE="${EMSCRIPTEN}/cmake/Modules/Platform/Emscripten.cmake" \
        -DCMAKE_PREFIX_PATH="${QT_WASM_PREFIX}" \
        -DCMAKE_BUILD_TYPE=Release
}

if command -v qt-cmake >/dev/null 2>&1; then
    configure_with_qt_cmake
else
    configure_with_cmake
fi

cmake --build "${BUILD_DIR}"
