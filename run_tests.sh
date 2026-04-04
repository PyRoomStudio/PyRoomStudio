#!/usr/bin/env bash
# Build the test suite and run all Qt tests headlessly.
# Usage: ./run_tests.sh [extra cmake args...]
#   e.g. ./run_tests.sh -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x/gcc_64
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build/test"

echo "==> Configuring (build dir: $BUILD_DIR)"
cmake -S "$SCRIPT_DIR" -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE=Debug \
    -G Ninja \
    "$@"

echo "==> Building"
cmake --build "$BUILD_DIR"

echo "==> Running tests"
export QT_QPA_PLATFORM=offscreen
ctest --test-dir "$BUILD_DIR" --output-on-failure
