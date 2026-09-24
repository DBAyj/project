#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
"$root/scripts/generate_p4_fixtures.sh"
"$root/scripts/test_p4.sh"
test -s "$root/assets/projection-fixtures/checkerboard.png"
test -s "$root/assets/projection-fixtures/homography-front.json"
file "$root/runtime/tmp/p4-build/services/astra-projection-service/astra-projection-service" | grep -q arm64
test -s "$root/protocols/projection/projection-render-request-v1.schema.json"
test -s "$root/protocols/projection/geometry-warp-v1.schema.json"
test -s "$root/docs/reports/p4-metal-verification.md"
test -s "$root/docs/reports/p4-performance-report.md"
test -s "$root/docs/reports/p4-stability-report.md"
cat > "$root/docs/reports/p4-verification-report.md" <<'EOF'
# P4 Verification Report

Result: `P4_VERIFICATION_PASSED`

The portable render library, projection runtime, capability-protected local
JSON-RPC service, Shell client/fallback, project-generated visual fixtures, and
macOS RHI/Metal evidence passed their P4 checks. The supporting graphics,
performance, and stability evidence is recorded in the neighboring P4 reports.

This verification is intentionally fixture-only:
`P3_REAL_SPATIAL_INTEGRATION_NOT_VERIFIED`.
It does not claim a physical projector, discovered target, camera calibration,
or real-space Homography validation.
EOF
echo "P4 verification passed: P3_REAL_SPATIAL_INTEGRATION_NOT_VERIFIED"
