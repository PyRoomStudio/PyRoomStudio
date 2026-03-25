#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_CANON="$(cd "${REPO_ROOT}" && pwd)"

QT_PREFIX_PATH="${QT_PREFIX_PATH:-}"
PROJECT_NAME="${PROJECT_NAME:-Seiche}"
BUILD_DIR="${BUILD_DIR:-build}"

cleanup_build=false
if [[ "${1:-}" == "--fresh" ]]; then
  cleanup_build=true
fi

# If `ninja` isn't available, ensure we don't keep an old `build/` directory
# configured with the Ninja generator (CMake will fail early when it can't
# find the Ninja executable). If the cache already uses a different generator,
# keep the build dir to speed up iteration.
cache_path="${REPO_ROOT}/${BUILD_DIR}/CMakeCache.txt"
if [[ -f "${cache_path}" ]]; then
  # Stale cache from another checkout or renamed folder (CMake refuses to reuse it).
  cached_home="$(awk -F= '/^CMAKE_HOME_DIRECTORY:INTERNAL=/{print $2; exit}' "${cache_path}" || true)"
  if [[ -n "${cached_home}" ]]; then
    cached_resolved="$(cd "${cached_home}" 2>/dev/null && pwd || true)"
    if [[ -z "${cached_resolved}" || "${cached_resolved}" != "${REPO_CANON}" ]]; then
      cleanup_build=true
    fi
  fi
fi

if ! command -v ninja >/dev/null 2>&1; then
  if [[ -f "${cache_path}" ]]; then
    cmake_generator="$(awk -F= '/^CMAKE_GENERATOR:INTERNAL=/{print $2; exit}' "${cache_path}" || true)"
    if [[ "${cmake_generator}" == "Ninja" ]]; then
      cleanup_build=true
    fi
  fi
fi

if [[ "$cleanup_build" == "true" ]]; then
  rm -rf "${REPO_ROOT:?}/${BUILD_DIR}"
fi

# If QT_PREFIX_PATH is not set, try to infer it from `macdeployqt`, then fall back to a common path.
if [[ -z "${QT_PREFIX_PATH}" ]]; then
  if command -v macdeployqt >/dev/null 2>&1; then
    QT_PREFIX_PATH="$(cd "$(dirname "$(command -v macdeployqt)")/.." && pwd)"
  fi
fi
if [[ -z "${QT_PREFIX_PATH}" ]]; then
  QT_PREFIX_PATH="${HOME}/Qt/6.10.2/macos"
fi

if [[ ! -d "${QT_PREFIX_PATH}" ]]; then
  echo "Error: QT_PREFIX_PATH does not exist: ${QT_PREFIX_PATH}" >&2
  echo "Set QT_PREFIX_PATH or install Qt under the default path (e.g. ~/Qt/6.x/macos)." >&2
  exit 1
fi

# Default generator: use Ninja only if it's actually available.
if [[ -z "${CMAKE_GENERATOR:-}" ]]; then
  if command -v ninja >/dev/null 2>&1; then
    CMAKE_GENERATOR="Ninja"
  else
    CMAKE_GENERATOR="Unix Makefiles"
  fi
fi
if [[ -n "${Qt6_DIR:-}" && -d "${Qt6_DIR}" ]]; then
  CMAKE_QT6_DIR_ARGS=( -DQt6_DIR="${Qt6_DIR}" )
else
  QT6_DIR_CANDIDATE="${QT_PREFIX_PATH}/lib/cmake/Qt6"
  if [[ -d "${QT6_DIR_CANDIDATE}" ]]; then
    CMAKE_QT6_DIR_ARGS=( -DQt6_DIR="${QT6_DIR_CANDIDATE}" )
  else
    CMAKE_QT6_DIR_ARGS=()
  fi
fi

cmake -S "${REPO_ROOT}" -B "${REPO_ROOT}/${BUILD_DIR}" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="${QT_PREFIX_PATH}" \
  -DCMAKE_DISABLE_FIND_PACKAGE_WrapVulkanHeaders=ON \
  -DCMAKE_FIND_PACKAGE_NO_PACKAGE_REGISTRY=ON \
  -DCMAKE_FIND_PACKAGE_NO_SYSTEM_PACKAGE_REGISTRY=ON \
  -G "${CMAKE_GENERATOR}" \
  "${CMAKE_QT6_DIR_ARGS[@]}"
cmake --build "${REPO_ROOT}/${BUILD_DIR}" --config Release --target "${PROJECT_NAME}"

APP_PATH=""

for dir in \
  "${REPO_ROOT}/${BUILD_DIR}/${PROJECT_NAME}.app" \
  "${REPO_ROOT}/${BUILD_DIR}/Release/${PROJECT_NAME}.app"; do
  if [[ -d "${dir}" ]]; then
    APP_PATH="${dir}"
    break
  fi
done
if [[ -z "${APP_PATH}" && -d "${REPO_ROOT}/${BUILD_DIR}" ]]; then
  while IFS= read -r -d '' app; do
    APP_PATH="${app}"
    break
  done < <(find "${REPO_ROOT}/${BUILD_DIR}" -name "${PROJECT_NAME}.app" -type d -print0 2>/dev/null)
fi

if [[ -z "${APP_PATH}" ]]; then
  echo "Error: macOS .app bundle not found in build dir: ${REPO_ROOT}/${BUILD_DIR}" >&2
  echo "This usually means the target is not built as a macOS bundle." >&2
  exit 1
fi

MACDEPLOYQT=""
if [[ -n "${QT_PREFIX_PATH}" && -x "${QT_PREFIX_PATH}/bin/macdeployqt" ]]; then
  MACDEPLOYQT="${QT_PREFIX_PATH}/bin/macdeployqt"
elif command -v macdeployqt >/dev/null 2>&1; then
  MACDEPLOYQT="$(command -v macdeployqt)"
fi

if [[ -n "${MACDEPLOYQT}" ]]; then
  "${MACDEPLOYQT}" "${APP_PATH}"
else
  echo "Warning: macdeployqt not found; app may require Qt installed on the target system." >&2
  echo "Set QT_PREFIX_PATH to your Qt install (e.g. /path/to/Qt/6.x/macos) to enable deployment." >&2
fi

echo "Build complete. Run: open \"${APP_PATH}\""
