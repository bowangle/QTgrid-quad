#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${BUILD_DIR:-$PROJECT_ROOT/build}"

echo "Writing test output to $BUILD_DIR/test-output.txt"
ctest --test-dir "$BUILD_DIR" --verbose --no-tests=error "$@" 2>&1 \
    | tee "$BUILD_DIR/test-output.txt"
