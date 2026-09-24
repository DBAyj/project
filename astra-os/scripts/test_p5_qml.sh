#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
"$root/scripts/build_p5.sh" >/dev/null
mkdir -p "$root/runtime/logs"
QT_QPA_PLATFORM=offscreen QT_QUICK_CONTROLS_STYLE=Basic \
  "$(brew --prefix qt)/bin/qmltestrunner" -input "$root/apps/astra-shell/tests/qml" \
  > "$root/runtime/logs/p5-qml-test.log" 2>&1
python3 "$root/scripts/verify_p5_qml_warnings.py" --log "$root/runtime/logs/p5-qml-test.log"
