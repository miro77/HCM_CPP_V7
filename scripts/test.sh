#!/usr/bin/env sh
# Build and run all tests (Linux / macOS / MSYS2).
#
#   scripts/test.sh [release|debug] [--clean]
set -eu
cd "$(dirname "$0")/.."

preset=release
for arg in "$@"; do
    case $arg in release|debug) preset=$arg ;; esac
done

sh scripts/build.sh "$@"
ctest --preset "$preset"
