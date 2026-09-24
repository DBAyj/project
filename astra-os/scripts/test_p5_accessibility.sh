#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
"$root/scripts/build_p5.sh" >/dev/null
mkdir -p "$root/runtime/logs"
QT_QPA_PLATFORM=offscreen QT_QUICK_CONTROLS_STYLE=Basic \
  "$(brew --prefix qt)/bin/qmltestrunner" -input "$root/apps/astra-shell/tests/qml" \
  > "$root/runtime/logs/p5-qml-test.log" 2>&1
python3 "$root/scripts/verify_p5_accessibility.py"
python3 "$root/scripts/verify_p5_qml_warnings.py" --log "$root/runtime/logs/p5-qml-test.log"
cat > "$root/docs/reports/p5-accessibility-report.md" <<'EOF'
# P5 Accessibility Evidence

Baseline: `P4_RELEASE_BASELINE_FINAL`

Result: `P5_ACCESSIBILITY_PASSED`

Seven core spatial controls and views expose semantic names, roles, and
descriptions. Interactive controls are keyboard focusable, focus outlines are
visible, and privacy/error states have text semantics. Validated service
preferences flow through `SpatialUIStateModel` into the QML workspace: Reduce
Motion resolves spatial transitions to 0 ms and High Contrast changes surfaces
plus the focus outline. Native, model, and QML tests cover the chain, and the
full QML suite reported zero project warnings.
EOF
