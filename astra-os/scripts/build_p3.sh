#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
"$root/scripts/configure_p3.sh"
cmake --build "$root/runtime/tmp/p1-build"
file "$root/runtime/tmp/p1-build/services/astra-spatial-service/astra-spatial-service" | grep -q arm64
file "$root/runtime/tmp/p1-build/apps/astra-shell/astra-shell" | grep -q arm64
echo "P3 build passed"
