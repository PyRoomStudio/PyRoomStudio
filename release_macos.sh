#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

QT_PREFIX_PATH="${QT_PREFIX_PATH:-}"
PROJECT_NAME="${PROJECT_NAME:-Seiche}"
BUILD_DIR="${BUILD_DIR:-build}"
DIST_DIR="${DIST_DIR:-dist}"

# Resolve QT_PREFIX_PATH before calling the build script so the child process
# inherits the same value (auto-detect from macdeployqt if not explicitly set).
if [[ -z "${QT_PREFIX_PATH}" ]]; then
  if command -v macdeployqt >/dev/null 2>&1; then
    QT_PREFIX_PATH="$(cd "$(dirname "$(command -v macdeployqt)")/.." && pwd)"
  fi
fi

if [[ -z "${QT_PREFIX_PATH}" ]]; then
  echo "Error: QT_PREFIX_PATH is not set and could not be inferred from macdeployqt." >&2
  echo "Set QT_PREFIX_PATH to your Qt macOS prefix (e.g. ~/Qt/6.x/macos)." >&2
  exit 1
fi

export QT_PREFIX_PATH

cleanup_build=false
if [[ "${1:-}" == "--fresh" ]]; then
  cleanup_build=true
fi

if [[ "$cleanup_build" == "true" ]]; then
  rm -rf "${REPO_ROOT:?}/${BUILD_DIR}"
fi

# Build via the shared build script (inherits exported QT_PREFIX_PATH)
"${REPO_ROOT}/build_macos.sh"

PROJECT_VERSION="$(sed -n 's/^project([^ ]* VERSION \([^ ]*\) LANGUAGES.*$/\1/p' "${REPO_ROOT}/CMakeLists.txt")"
if [[ -z "${PROJECT_VERSION}" ]]; then
  PROJECT_VERSION="0.0.0"
fi

STAGE_DIR="${REPO_ROOT}/${DIST_DIR}/${PROJECT_NAME}-${PROJECT_VERSION}-macos"
ARCHIVE_PATH="${STAGE_DIR}.zip"

rm -rf "${STAGE_DIR}" "${ARCHIVE_PATH}"
mkdir -p "${STAGE_DIR}"

APP_PATH=""

# Common locations depending on generator / CMake build layout
shopt -s nullglob
for candidate in \
  "${REPO_ROOT}/${BUILD_DIR}/${PROJECT_NAME}.app" \
  "${REPO_ROOT}/${BUILD_DIR}/Release/${PROJECT_NAME}.app" \
  "${REPO_ROOT}/${BUILD_DIR}"/${PROJECT_NAME}.app \
  "${REPO_ROOT}/${BUILD_DIR}"/*/"${PROJECT_NAME}.app" \
  "${REPO_ROOT}/${BUILD_DIR}"/*/"${PROJECT_NAME}.app"/Contents/MacOS/*; do
  # If the glob hits the nested Contents/MacOS path, normalize back to the app bundle root.
  if [[ "${candidate}" == *"/${PROJECT_NAME}.app/Contents/MacOS/"* ]]; then
    candidate="${candidate%/Contents/MacOS/*}"
  fi
  if [[ -d "${candidate}" && "${candidate}" == *".app" ]]; then
    APP_PATH="${candidate}"
    break
  fi
done
shopt -u nullglob

if [[ -z "${APP_PATH}" ]]; then
  echo "Error: macOS .app bundle not found in build dir: ${REPO_ROOT}/${BUILD_DIR}" >&2
  echo "This usually means the target is not built as a macOS bundle." >&2
  exit 1
fi

cp -R "${APP_PATH}" "${STAGE_DIR}/"
APP_BUNDLE="${STAGE_DIR}/${PROJECT_NAME}.app"
mkdir -p "${APP_BUNDLE}/Contents/MacOS"
# macOS runtime expects `materials/` next to the executable (Contents/MacOS/materials).
cp -R "${REPO_ROOT}/materials" "${APP_BUNDLE}/Contents/MacOS/"

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

