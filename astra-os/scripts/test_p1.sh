#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
"$root/scripts/build_p1.sh"
ctest --test-dir "$root/runtime/tmp/p1-build" --output-on-failure
"$root/.venv/bin/python" "$root/tests/contract/test_p1_schemas.py"
python3 -m unittest "$root/tests/p1-e2e/test_p1_graphics_backend.py"
QT_QPA_PLATFORM=offscreen \
QT_QUICK_CONTROLS_STYLE=Basic \
"$(brew --prefix qt)/bin/qmltestrunner" \
  -input "$root/apps/astra-shell/tests/qml" \
  -silent
