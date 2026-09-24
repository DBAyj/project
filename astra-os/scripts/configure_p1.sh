#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
build="$root/runtime/tmp/p1-build"
cmake -S "$root" -B "$build" -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH="$(brew --prefix qt)" -DCMAKE_OSX_ARCHITECTURES=arm64
