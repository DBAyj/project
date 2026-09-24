#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
"$root/scripts/configure_p4.sh"
cmake --build "$root/runtime/tmp/p4-build"
file "$root/runtime/tmp/p4-build/services/astra-projection-service/astra-projection-service"
file "$root/runtime/tmp/p4-build/apps/astra-shell/astra-shell"
