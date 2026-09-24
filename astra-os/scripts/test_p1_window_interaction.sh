#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
"$root/scripts/build_p1.sh" >/dev/null
ctest --test-dir "$root/runtime/tmp/p1-build" -R '^astra-shell-window-interaction-tests$' --output-on-failure
QT_QPA_PLATFORM=offscreen \
QT_QUICK_CONTROLS_STYLE=Basic \
  "$(brew --prefix qt)/bin/qmltestrunner" -input "$root/apps/astra-shell/tests/qml" -silent
"$root/scripts/test_p1_metal.sh"
(
  cd "$root"
  python3 scripts/verify_p1_window_interaction.py
)
