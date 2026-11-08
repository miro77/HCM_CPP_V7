#!/usr/bin/env sh
# Build and test (or lint) inside a Linux container.
#
#   scripts/docker.sh [test|lint|all]     (default: test)
set -eu
cd "$(dirname "$0")/.."

target=${1:-test}
case $target in
    test|lint) docker build --target "$target" -t "hcmms:$target" . ;;
    all) docker build --target test -t hcmms:test . && docker build --target lint -t hcmms:lint . ;;
    *) echo "usage: $0 [test|lint|all]" >&2; exit 2 ;;
esac
