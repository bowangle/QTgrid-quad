#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd -- "$PROJECT_ROOT/../.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$PROJECT_ROOT/build}"

# The numeric layer must already be installed. Extra arguments override defaults:
# ./compile.sh -DCMAKE_PREFIX_PATH=/path/to/install -DQTGRID_BUILD_TESTS=OFF
cmake -S "$PROJECT_ROOT" -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH="$REPO_ROOT/install" "$@"
cmake --build "$BUILD_DIR" --parallel "${BUILD_JOBS:-2}"
