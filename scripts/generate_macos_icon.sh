#!/usr/bin/env bash
# Regenerate resources/Seiche.icns from assets/logo.svg (run on macOS).
set -euo pipefail
REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TMP="${REPO_ROOT}/resources/.icon_gen"
ICNS_OUT="${REPO_ROOT}/resources/Seiche.icns"
rm -rf "${TMP}"
mkdir -p "${TMP}/Seiche.iconset"
qlmanage -t -s 1024 -o "${TMP}" "${REPO_ROOT}/assets/logo.svg"
MASTER="${TMP}/logo.svg.png"
if [[ ! -f "${MASTER}" ]]; then
  echo "error: expected ${MASTER} from qlmanage" >&2
  exit 1
fi
# Inset the logo on a 1024² transparent canvas so Finder/Dock squircle shows a bit more curve.
# Requires ImageMagick (`brew install imagemagick`); if missing, the raw qlmanage raster is used.
if command -v magick >/dev/null 2>&1; then
  magick "${MASTER}" -resize 86% -gravity center -background none -extent 1024x1024 "${TMP}/master_inset.png"
  MASTER="${TMP}/master_inset.png"
fi
sips -z 16 16 "${MASTER}" --out "${TMP}/Seiche.iconset/icon_16x16.png"
sips -z 32 32 "${MASTER}" --out "${TMP}/Seiche.iconset/icon_16x16@2x.png"
sips -z 32 32 "${MASTER}" --out "${TMP}/Seiche.iconset/icon_32x32.png"
sips -z 64 64 "${MASTER}" --out "${TMP}/Seiche.iconset/icon_32x32@2x.png"
sips -z 128 128 "${MASTER}" --out "${TMP}/Seiche.iconset/icon_128x128.png"
sips -z 256 256 "${MASTER}" --out "${TMP}/Seiche.iconset/icon_128x128@2x.png"
sips -z 256 256 "${MASTER}" --out "${TMP}/Seiche.iconset/icon_256x256.png"
sips -z 512 512 "${MASTER}" --out "${TMP}/Seiche.iconset/icon_256x256@2x.png"
sips -z 512 512 "${MASTER}" --out "${TMP}/Seiche.iconset/icon_512x512.png"
sips -z 1024 1024 "${MASTER}" --out "${TMP}/Seiche.iconset/icon_512x512@2x.png"
iconutil -c icns "${TMP}/Seiche.iconset" -o "${ICNS_OUT}"
rm -rf "${TMP}"
echo "Wrote ${ICNS_OUT}"
