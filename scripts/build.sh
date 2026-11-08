#!/usr/bin/env sh
# Configure and build the C++ library, CLI and tests (Linux / macOS / MSYS2).
#
#   scripts/build.sh [release|debug] [--clean]
#
# Uses Ninja if it is installed, otherwise the CMake default generator.
# Output: build/<preset>/
set -eu
cd "$(dirname "$0")/.."

preset=release
clean=0
for arg in "$@"; do
    case $arg in
        release|debug) preset=$arg ;;
        --clean) clean=1 ;;
        -h|--help) sed -n '2,8p' "$0"; exit 0 ;;
        *) echo "unknown argument: $arg" >&2; exit 2 ;;
    esac
done

[ "$clean" = 1 ] && rm -rf "build/$preset"

# The generator is fixed at the first configure; only pick one for a fresh tree.
if [ ! -f "build/$preset/CMakeCache.txt" ] && command -v ninja >/dev/null 2>&1; then
    cmake --preset "$preset" -G Ninja
else
    cmake --preset "$preset"
fi
cmake --build --preset "$preset" --parallel
