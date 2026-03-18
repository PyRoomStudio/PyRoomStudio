#!/usr/bin/env bash
set -euo pipefail

QT_ROOT="C:/Qt/6.10.2/mingw_64"
MINGW_BIN="C:/Qt/Tools/mingw1310_64/bin"

rm -rf build

cmake -S . -B build \
  -DCMAKE_PREFIX_PATH="$QT_ROOT" \
  -DQt6_DIR="$QT_ROOT/lib/cmake/Qt6" \
  -DCMAKE_CXX_COMPILER="$MINGW_BIN/g++.exe" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_DISABLE_FIND_PACKAGE_WrapVulkanHeaders=ON \
  -DCMAKE_FIND_PACKAGE_NO_PACKAGE_REGISTRY=ON \
  -DCMAKE_FIND_PACKAGE_NO_SYSTEM_PACKAGE_REGISTRY=ON \
  -G Ninja

cmake --build build

# Deploy Qt DLLs/plugins next to the .exe so it runs
if [[ -x "${QT_ROOT}/bin/windeployqt.exe" ]]; then
  "${QT_ROOT}/bin/windeployqt.exe" --release "build/PyRoomStudio.exe"
else
  echo "Warning: windeployqt.exe not found at '${QT_ROOT}/bin/windeployqt.exe'"
  echo "PyRoomStudio.exe may fail to start unless Qt's bin is on PATH."
fi

echo "Build complete. Run: ./build/PyRoomStudio.exe"