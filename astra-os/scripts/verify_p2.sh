#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
"$root/scripts/test_p2.sh"
"$root/scripts/test_p2.sh" performance
"$root/scripts/run_p2.sh"
trap '"$root/scripts/stop_p2.sh" >/dev/null 2>&1 || true' EXIT
"$root/.venv/bin/python" "$root/tests/p2-e2e/test_p2_live_service.py"
"$root/scripts/stop_p2.sh"
trap - EXIT
"$root/.venv/bin/python" "$root/scripts/verify_p2_baseline.py"
file "$root/runtime/tmp/p1-build/apps/astra-shell/astra-shell" | grep -q arm64
echo "P2 verification passed"
