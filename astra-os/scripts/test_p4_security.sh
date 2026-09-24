#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
"$root/scripts/build_p4.sh"
ctest --test-dir "$root/runtime/tmp/p4-build" -R '^(astra-projection-safe-clear|astra-projection-runtime|astra-projection-service)-tests$' --output-on-failure
