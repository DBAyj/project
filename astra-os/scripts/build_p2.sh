#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
"$root/scripts/configure_p2.sh"
"$root/.venv/bin/python" -m compileall -q "$root/services/astra-intent-service/src"
"$root/scripts/build_p1.sh"
binary="$root/runtime/tmp/p1-build/apps/astra-shell/astra-shell"
file "$binary" | grep -q arm64
echo "P2 build passed service=astra-intent-service shell=$binary"
