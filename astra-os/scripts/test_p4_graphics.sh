#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
"$root/scripts/stop_p4.sh" || true
trap '"$root/scripts/stop_p4.sh"' EXIT
"$root/scripts/run_p4.sh"
sleep 3
(
  cd "$root"
  python3 scripts/verify_p4_graphics.py
)
cat > "$root/docs/reports/p4-metal-verification.md" <<'EOF'
# P4 Metal Verification

Result: `P4_METAL_CONFIRMED`

The runtime log recorded Metal through Qt RHI for the Phone and Projection
windows, then recorded the P4 fixture PNG as ready in the visible Projection
window. This is a macOS development-host graphics result; the renderer source
remains portable to ARM64 Linux through Qt RHI.

Spatial evidence remains fixture-only:
`P3_REAL_SPATIAL_INTEGRATION_NOT_VERIFIED`.
EOF
