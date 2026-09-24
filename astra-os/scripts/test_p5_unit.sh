#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
"$root/scripts/build_p5.sh" >/dev/null
ctest --test-dir "$root/runtime/tmp/p5-build" \
  -R '^(astra-ui-(component|window|layout|focus|input|interaction|task-surface|notification|privacy|state-store|animation-accessibility)-tests|astra-spatial-ui-(service|runtime-command|runtime-projection|runtime-state)-tests)$' \
  --output-on-failure
