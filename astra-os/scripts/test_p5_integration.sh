#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
"$root/scripts/build_p5.sh" >/dev/null
ctest --test-dir "$root/runtime/tmp/p5-build" \
  -R '^(astra-shell-spatial-ui-client-model-tests|astra-shell-p5-spatial-ui-integration-tests|astra-shell-p5-spatial-target-chain-tests)$' \
  --output-on-failure
