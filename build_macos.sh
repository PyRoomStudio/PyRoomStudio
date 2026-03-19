#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

QT_PREFIX_PATH="${QT_PREFIX_PATH:-${HOME}/Qt/6.10.2/macos}"
PROJECT_NAME="${PROJECT_NAME:-PyRoomStudio}"
BUILD_DIR="${BUILD_DIR:-build}"

cleanup_build=false
if [[ "${1:-}" == "--fresh" ]]; then
  cleanup_build=true
fi

if [[ "$cleanup_build" == "true" ]]; then
  rm -rf "${REPO_ROOT:?}/${BUILD_DIR}"
fi

GENERATOR="Unix Makefiles"
if command -v ninja >/dev/null 2>&1; then
  GENERATOR="Ninja"
fi

cmake -S "${REPO_ROOT}" -B "${REPO_ROOT}/${BUILD_DIR}" \
  -DCMAKE_PREFIX_PATH="${QT_PREFIX_PATH}" \
  -DCMAKE_BUILD_TYPE=Release \
  -G "${GENERATOR}"

cmake --build "${REPO_ROOT}/${BUILD_DIR}" --config Release
