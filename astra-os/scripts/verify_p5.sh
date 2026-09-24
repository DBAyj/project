#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
marker="P4_RELEASE_BASELINE_FINAL"
"$root/scripts/generate_spatial_ui_fixtures.sh"
"$root/scripts/test_p5.sh"

make -C "$root" p2-test
make -C "$root" p2-verify
make -C "$root" p3-test
make -C "$root" p3-verify
make -C "$root" p4-test
make -C "$root" p4-verify
"$root/.venv/bin/python" "$root/scripts/verify_release_chain.py"

test "$(find "$root/assets/spatial-ui-fixtures" -name '*.json' -type f | wc -l | tr -d ' ')" = 11
test -s "$root/runtime/tmp/p5-build/services/astra-spatial-ui-service/astra-spatial-ui-service"
test -s "$root/runtime/tmp/p5-build/apps/astra-shell/astra-shell"
file "$root/runtime/tmp/p5-build/services/astra-spatial-ui-service/astra-spatial-ui-service" | grep -q arm64
for schema in "$root"/protocols/spatial-ui/*.schema.json "$root"/schemas/spatial-*.schema.json; do
  test -s "$schema"
done
for config in "$root"/config/spatial-{ui,layout,input,accessibility,theme}.yaml; do
  test -s "$config"
done
for report in \
  p5-preflight-audit.md p5-build-report.md p5-test-report.md \
  p5-accessibility-report.md p5-graphics-report.md p5-interaction-report.md \
  p5-security-report.md p5-performance-report.md p5-stability-report.md; do
  test -s "$root/docs/reports/$report"
  grep -q "$marker" "$root/docs/reports/$report"
done
python3 - "$root/assets/spatial-ui-fixtures" "$marker" <<'PY'
import json
import sys
from pathlib import Path

directory = Path(sys.argv[1])
marker = sys.argv[2]
for fixture in directory.glob("*.json"):
    assert json.loads(fixture.read_text(encoding="utf-8"))["p4_release_status"] == marker
print("P5 fixture markers verified")
PY
cat > "$root/docs/reports/p5-verification-report.md" <<EOF
# P5 Verification Report

Baseline: \`P4_RELEASE_BASELINE_FINAL\`

Result: \`P5_VERIFICATION_PASSED\`

The independent service, portable UI library, Shell runtime component tree,
per-method schemas, 11 deterministic fixtures, notification lifecycle,
interaction persistence, P4 layer submission, target-loss safe clear,
native/contract/QML tests, and specialized P5 evidence checks passed. P2, P3,
and P4 regressions, the P3 service ProjectionTarget chain, and the formal P1-P4
release-tag ancestry also passed.
EOF
echo "P5_VERIFICATION_PASSED"
