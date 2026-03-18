#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

QT_PREFIX_PATH="${QT_PREFIX_PATH:-}"
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

# Build (generic CMake/Ninja build; override QT_PREFIX_PATH/BUILD_DIR as needed)
cmake -S "${REPO_ROOT}" -B "${REPO_ROOT}/${BUILD_DIR}" \
  -DCMAKE_BUILD_TYPE=Release \
  ${QT_PREFIX_PATH:+-DCMAKE_PREFIX_PATH="${QT_PREFIX_PATH}"} \
  -G Ninja
cmake --build "${REPO_ROOT}/${BUILD_DIR}" --config Release

PROJECT_VERSION="$(sed -n 's/^project(PyRoomStudio VERSION \([^ ]*\) LANGUAGES.*$/\1/p' "${REPO_ROOT}/CMakeLists.txt")"
if [[ -z "${PROJECT_VERSION}" ]]; then
  PROJECT_VERSION="0.0.0"
fi

STAGE_DIR="${REPO_ROOT}/${DIST_DIR}/${PROJECT_NAME}-${PROJECT_VERSION}-linux"
ARCHIVE_PATH="${STAGE_DIR}.tar.gz"

EXE_PATH="${REPO_ROOT}/${BUILD_DIR}/${PROJECT_NAME}"
if [[ ! -f "${EXE_PATH}" ]]; then
  echo "Error: executable not found at: ${EXE_PATH}" >&2
  exit 1
fi

rm -rf "${STAGE_DIR}" "${ARCHIVE_PATH}"
mkdir -p "${STAGE_DIR}"

cp -f "${EXE_PATH}" "${STAGE_DIR}/"
cp -R "${REPO_ROOT}/materials" "${STAGE_DIR}/materials"

# Optional: bundle Qt libs/plugins if linuxdeployqt is available
LINUXDEPLOYQT=""
if command -v linuxdeployqt >/dev/null 2>&1; then
  LINUXDEPLOYQT="$(command -v linuxdeployqt)"
elif [[ -n "${QT_PREFIX_PATH}" && -x "${QT_PREFIX_PATH}/bin/linuxdeployqt" ]]; then
  LINUXDEPLOYQT="${QT_PREFIX_PATH}/bin/linuxdeployqt"
fi

if [[ -n "${LINUXDEPLOYQT}" ]]; then
  (
    cd "${STAGE_DIR}"
    # AppDir layout: keep it simple and let linuxdeployqt do its best.
    mkdir -p usr/bin
    mv "./${PROJECT_NAME}" usr/bin/
    "${LINUXDEPLOYQT}" "usr/bin/${PROJECT_NAME}" -bundle-non-qt-libs || true
    mv usr/bin/"${PROJECT_NAME}" ./
    rm -rf usr
  )
else
  echo "Warning: linuxdeployqt not found; your binary may require Qt available on the target machine." >&2
fi

tar -C "${REPO_ROOT}/${DIST_DIR}" -czf "${ARCHIVE_PATH}" "$(basename "${STAGE_DIR}")"

echo "Release created:"
echo "  ${ARCHIVE_PATH}"

