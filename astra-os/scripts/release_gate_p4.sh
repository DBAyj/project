#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
report="$root/docs/reports/p4-release-gate-report.md"
"$root/scripts/stop_p4.sh" || true
trap '"$root/scripts/stop_p4.sh"' EXIT
make -C "$root" format
make -C "$root" lint
make -C "$root" p1-test
make -C "$root" p1-verify
make -C "$root" p4-generate-fixtures
make -C "$root" p4-configure
make -C "$root" p4-build
make -C "$root" p4-test
make -C "$root" p4-test-integration
make -C "$root" p4-test-security
make -C "$root" p4-test-graphics
make -C "$root" p4-test-visual
make -C "$root" p4-test-performance
make -C "$root" p4-test-stability
make -C "$root" p4-verify
make -C "$root" docs-verify
"$root/scripts/run_p4.sh"
sleep 2
"$root/scripts/stop_p4.sh"
cat > "$report" <<'EOF'
# P4 Release Gate Report

Result: `P4_RELEASE_GATE_PASSED`

The gate ran P1 regression, P4 fixture generation, contracts, unit and
integration tests, safe-clear security tests, actual Metal/RHI evidence,
visual baselines, performance evidence, stability cycles, verification, and an
actual project-owned P4 run/stop cycle.

Spatial evidence remains fixture-only:
`P3_REAL_SPATIAL_INTEGRATION_NOT_VERIFIED`.
No physical projector, target discovery, live camera calibration, or real-space
Homography result is claimed by this gate.
EOF
echo "P4_RELEASE_GATE_PASSED"
