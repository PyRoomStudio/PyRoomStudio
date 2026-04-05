#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

QT_PREFIX_PATH="${QT_PREFIX_PATH:-C:/Qt/6.10.2/mingw_64}"
PROJECT_NAME="${PROJECT_NAME:-Seiche}"
BUILD_DIR="${BUILD_DIR:-build}"
DIST_DIR="${DIST_DIR:-dist}"

cleanup_build=false
if [[ "${1:-}" == "--fresh" ]]; then
  cleanup_build=true
fi

if [[ "$cleanup_build" == "true" ]]; then
  rm -rf "${REPO_ROOT}/${BUILD_DIR}"
fi

"${REPO_ROOT}/build_windows.sh"

PROJECT_VERSION="$(sed -n 's/^project([^ ]* VERSION \([^ ]*\) LANGUAGES.*$/\1/p' "${REPO_ROOT}/CMakeLists.txt")"
if [[ -z "${PROJECT_VERSION}" ]]; then
  PROJECT_VERSION="0.0.0"
fi

STAGE_DIR="${REPO_ROOT}/${DIST_DIR}/${PROJECT_NAME}-${PROJECT_VERSION}-windows"
ZIP_PATH="${STAGE_DIR}.zip"

EXE_PATH="${REPO_ROOT}/${BUILD_DIR}/${PROJECT_NAME}.exe"
if [[ ! -f "${EXE_PATH}" ]]; then
  echo "Error: executable not found at: ${EXE_PATH}" >&2
  echo "Make sure the Release build succeeded (script calls ./build_windows.sh)." >&2
  exit 1
fi

if ! command -v zip >/dev/null 2>&1; then
  echo "Error: 'zip' not found in PATH. Install zip or create archives manually." >&2
  exit 1
fi

rm -rf "${STAGE_DIR}" "${ZIP_PATH}"
mkdir -p "${STAGE_DIR}"

cp -f "${EXE_PATH}" "${STAGE_DIR}/"
rm -rf "${STAGE_DIR}/materials"
cp -R "${REPO_ROOT}/materials" "${STAGE_DIR}/materials"

WINDEPLOYQT="${WINDEPLOYQT:-${QT_PREFIX_PATH}/bin/windeployqt6.exe}"
if [[ ! -f "$WINDEPLOYQT" ]]; then
  # Fallback to windeployqt (some Qt installs expose only one binary name)
  if [[ -f "${QT_PREFIX_PATH}/bin/windeployqt.exe" ]]; then
    WINDEPLOYQT="${QT_PREFIX_PATH}/bin/windeployqt.exe"
  fi
fi

if [[ ! -f "$WINDEPLOYQT" ]]; then
  echo "Error: windeployqt not found. Set WINDEPLOYQT or QT_PREFIX_PATH." >&2
  echo "Tried: $WINDEPLOYQT" >&2
  exit 1
fi

(
  cd "${STAGE_DIR}"
  "${WINDEPLOYQT}" "./${PROJECT_NAME}.exe"
)

# Copy MinGW runtime DLLs that windeployqt doesn't handle
MINGW_BIN="${MINGW_BIN:-C:/Qt/Tools/mingw1310_64/bin}"
for dll in libgomp-1.dll libgcc_s_seh-1.dll libstdc++-6.dll libwinpthread-1.dll; do
  if [[ -f "${MINGW_BIN}/${dll}" ]]; then
    cp -f "${MINGW_BIN}/${dll}" "${STAGE_DIR}/"
  else
    echo "Warning: ${dll} not found in ${MINGW_BIN}" >&2
  fi
done

# Create zip for upload (zip creates ${ZIP_PATH} from STAGE_DIR content)
(
  cd "${REPO_ROOT}/${DIST_DIR}"
  zip -r "$(basename "${ZIP_PATH}")" "$(basename "${STAGE_DIR}")"
)

echo "Release created:"
echo "  ${ZIP_PATH}"

