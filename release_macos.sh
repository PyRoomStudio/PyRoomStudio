#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

DEFAULT_QT_PREFIX_PATH="${HOME}/Qt/6.10.2/macos"
QT_PREFIX_PATH="${QT_PREFIX_PATH:-${DEFAULT_QT_PREFIX_PATH}}"
PROJECT_NAME="${PROJECT_NAME:-PyRoomStudio}"
BUILD_DIR="${BUILD_DIR:-build}"
DIST_DIR="${DIST_DIR:-dist}"

cleanup_build=false
if [[ "${1:-}" == "--fresh" ]]; then
  cleanup_build=true
fi

if [[ "$cleanup_build" == "true" ]]; then
  rm -rf "${REPO_ROOT:?}/${BUILD_DIR}"
fi

# Build (expects build_macos.sh to be configured for your Qt path)
QT_PREFIX_PATH="${QT_PREFIX_PATH}" BUILD_DIR="${BUILD_DIR}" PROJECT_NAME="${PROJECT_NAME}" "${REPO_ROOT}/build_macos.sh"

PROJECT_VERSION="$(sed -n 's/^project(PyRoomStudio VERSION \([^ ]*\) LANGUAGES.*$/\1/p' "${REPO_ROOT}/CMakeLists.txt")"
if [[ -z "${PROJECT_VERSION}" ]]; then
  PROJECT_VERSION="0.0.0"
fi

STAGE_DIR="${REPO_ROOT}/${DIST_DIR}/${PROJECT_NAME}-${PROJECT_VERSION}-macos"
ARCHIVE_PATH="${STAGE_DIR}.zip"

rm -rf "${STAGE_DIR}" "${ARCHIVE_PATH}"
mkdir -p "${STAGE_DIR}"

APP_PATH="${REPO_ROOT}/${BUILD_DIR}/${PROJECT_NAME}.app"
if [[ ! -d "${APP_PATH}" ]]; then
  echo "Error: macOS .app bundle not found at: ${APP_PATH}" >&2
  echo "Make sure you are building with an Xcode/Unix Makefiles generator that produces .app bundles," >&2
  echo "or update BUILD_DIR/your build script accordingly." >&2
  exit 1
fi

cp -R "${APP_PATH}" "${STAGE_DIR}/"
cp -R "${REPO_ROOT}/materials" "${STAGE_DIR}/materials"

# Deploy Qt frameworks into the app bundle if macdeployqt is available
MACDEPLOYQT=""
if [[ -n "${QT_PREFIX_PATH}" && -x "${QT_PREFIX_PATH}/bin/macdeployqt" ]]; then
  MACDEPLOYQT="${QT_PREFIX_PATH}/bin/macdeployqt"
elif command -v macdeployqt >/dev/null 2>&1; then
  MACDEPLOYQT="$(command -v macdeployqt)"
fi

if [[ -n "${MACDEPLOYQT}" ]]; then
  "${MACDEPLOYQT}" "${STAGE_DIR}/${PROJECT_NAME}.app"
else
  echo "Warning: macdeployqt not found; app may require Qt installed on the target system." >&2
  echo "Set QT_PREFIX_PATH to your Qt install (e.g. /path/to/Qt/6.x/macos) to enable deployment." >&2
fi

if ! command -v zip >/dev/null 2>&1; then
  echo "Error: 'zip' not found in PATH. Install zip or archive ${STAGE_DIR} manually." >&2
  exit 1
fi

(
  cd "${REPO_ROOT}/${DIST_DIR}"
  zip -r "$(basename "${ARCHIVE_PATH}")" "$(basename "${STAGE_DIR}")"
)

echo "Release created:"
echo "  ${ARCHIVE_PATH}"

