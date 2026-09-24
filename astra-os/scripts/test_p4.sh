#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
"$root/scripts/build_p4.sh"
ctest --test-dir "$root/runtime/tmp/p4-build" -R '^(astra-render|astra-projection|astra-shell-p4-projection-integration|astra-shell-projection-service-client)-' --output-on-failure
"$root/.venv/bin/python" "$root/tests/contract/test_p4_schemas.py"
python3 "$root/tools/projection-fixture-generator/test_generate.py"
python3 "$root/tests/p4-e2e/test_p4_graphics.py"
python3 "$root/tests/p4-e2e/test_p4_visual.py"
