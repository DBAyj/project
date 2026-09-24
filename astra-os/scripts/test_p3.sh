#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
"$root/scripts/build_p3.sh" >/dev/null
ctest --test-dir "$root/runtime/tmp/p1-build" -R '^astra-spatial-(domain|frame-source|vision|calibration|service|jsonrpc|camera-evidence|performance|stability)-tests$' --output-on-failure
"$root/.venv/bin/python" "$root/tests/contract/test_p3_schemas.py" -v
python3.12 -m unittest "$root/tools/spatial-fixture-generator/tests/test_fixture_generator.py"
