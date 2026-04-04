#!/usr/bin/env bash
# Build a release archive for Linux.
# Usage:
#   ./release_linux.sh [--fresh]
#
# Environment variables:
#   QT_PREFIX_PATH   Path to the Qt prefix (e.g. /path/to/Qt/6.x/gcc_64).
#                    Auto-detected from qmake6/qmake if not set.
#   LINUXDEPLOYQT    Path to linuxdeployqt binary (optional). When present,
#                    Qt .so files are bundled into the stage directory.
#   PROJECT_NAME     Defaults to "Seiche".
#   BUILD_DIR        Defaults to "build".
#   DIST_DIR         Defaults to "dist".
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

QT_PREFIX_PATH="${QT_PREFIX_PATH:-}"
PROJECT_NAME="${PROJECT_NAME:-Seiche}"
BUILD_DIR="${BUILD_DIR:-build}"
DIST_DIR="${DIST_DIR:-dist}"

cleanup_build=false
if [[ "${1:-}" == "--fresh" ]]; then
  cleanup_build=true
fi

if [[ "$cleanup_build" == "true" ]]; then
  rm -rf "${REPO_ROOT:?}/${BUILD_DIR}"
fi

# -- Resolve Qt prefix ---------------------------------------------------------
if [[ -z "${QT_PREFIX_PATH}" ]]; then
  if command -v qmake6 >/dev/null 2>&1; then
    QT_PREFIX_PATH="$(qmake6 -query QT_INSTALL_PREFIX)"
  elif command -v qmake >/dev/null 2>&1; then
    QT_PREFIX_PATH="$(qmake -query QT_INSTALL_PREFIX)"
  fi
fi

# -- Build ---------------------------------------------------------------------
CMAKE_EXTRA_ARGS=()
if [[ -n "${QT_PREFIX_PATH}" ]]; then
  CMAKE_EXTRA_ARGS+=(-DCMAKE_PREFIX_PATH="${QT_PREFIX_PATH}")
fi

cmake -S "${REPO_ROOT}" -B "${REPO_ROOT}/${BUILD_DIR}" \
  -DCMAKE_BUILD_TYPE=Release \
  -G Ninja \
  "${CMAKE_EXTRA_ARGS[@]}"
cmake --build "${REPO_ROOT}/${BUILD_DIR}" --config Release

# -- Stage ---------------------------------------------------------------------
PROJECT_VERSION="$(sed -n 's/^project([^ ]* VERSION \([^ ]*\) LANGUAGES.*$/\1/p' "${REPO_ROOT}/CMakeLists.txt")"
if [[ -z "${PROJECT_VERSION}" ]]; then
  PROJECT_VERSION="0.0.0"
fi

STAGE_DIR="${REPO_ROOT}/${DIST_DIR}/${PROJECT_NAME}-${PROJECT_VERSION}-linux"
ARCHIVE_PATH="${STAGE_DIR}.tar.gz"

rm -rf "${STAGE_DIR}" "${ARCHIVE_PATH}"
mkdir -p "${STAGE_DIR}"

BIN_PATH="${REPO_ROOT}/${BUILD_DIR}/${PROJECT_NAME}"
if [[ ! -f "${BIN_PATH}" ]]; then
  echo "Error: executable not found at: ${BIN_PATH}" >&2
  echo "Make sure the Release build succeeded." >&2
  exit 1
fi

cp -f "${BIN_PATH}" "${STAGE_DIR}/"
cp -R "${REPO_ROOT}/materials" "${STAGE_DIR}/materials"

# -- Bundle Qt libraries (optional, via linuxdeployqt) -------------------------
LINUXDEPLOYQT="${LINUXDEPLOYQT:-}"
if [[ -z "${LINUXDEPLOYQT}" ]] && command -v linuxdeployqt >/dev/null 2>&1; then
  LINUXDEPLOYQT="$(command -v linuxdeployqt)"
fi

if [[ -n "${LINUXDEPLOYQT}" && -x "${LINUXDEPLOYQT}" ]]; then
  "${LINUXDEPLOYQT}" "${STAGE_DIR}/${PROJECT_NAME}" -bundle-non-qt-libs
else
  echo "Note: linuxdeployqt not found — Qt libraries will not be bundled." >&2
  echo "Set LINUXDEPLOYQT= or install it to produce a self-contained archive." >&2
  echo "Users must have Qt 6 installed on their system." >&2
fi

# -- Archive -------------------------------------------------------------------
mkdir -p "${REPO_ROOT}/${DIST_DIR}"
(
  cd "${REPO_ROOT}/${DIST_DIR}"
  tar -czf "$(basename "${ARCHIVE_PATH}")" "$(basename "${STAGE_DIR}")"
)

echo "Release created:"
echo "  ${ARCHIVE_PATH}"
