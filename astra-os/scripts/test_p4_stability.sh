#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
socket="$root/runtime/state/p4-projection.sock"
"$root/scripts/stop_p4.sh" || true
trap '"$root/scripts/stop_p4.sh"' EXIT
"$root/scripts/run_p4.sh"
sleep 1
"$root/.venv/bin/python" "$root/scripts/p4_service_client.py" --socket "$socket" --cycles 100
"$root/scripts/stop_p4.sh"
test ! -e "$root/runtime/state/p4-projection-service.pid"
test ! -e "$root/runtime/state/p4-shell.pid"
test ! -e "$root/runtime/state/p4-projection.sock"
test ! -e "$root/runtime/cache/p4-output.png"
cat > "$root/docs/reports/p4-stability-report.md" <<'EOF'
# P4 Stability Evidence

Result: `P4_STABILITY_PASSED`

The project-owned projection service completed 100 `INITIALIZE -> READY ->
RENDER -> STOP` fixture cycles. Each stop used the safe-clear path; the client
removes its cached output PNG after each stop so no stale frame remains in the
Shell display path.

Spatial evidence remains fixture-only:
`P3_REAL_SPATIAL_INTEGRATION_NOT_VERIFIED`.
EOF
