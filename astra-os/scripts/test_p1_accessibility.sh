#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
QT_QPA_PLATFORM=offscreen \
QT_QUICK_CONTROLS_STYLE=Basic \
  "$(brew --prefix qt)/bin/qmltestrunner" -input "$root/apps/astra-shell/tests/qml" -silent
"$root/scripts/test_p1_metal.sh"
(
  cd "$root"
  python3 scripts/check_p1_qml_warnings.py
)
