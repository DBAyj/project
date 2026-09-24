#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
"$root/scripts/configure_p1.sh"
cmake --build "$root/runtime/tmp/p1-build"
file "$root/runtime/tmp/p1-build/apps/astra-shell/astra-shell"
