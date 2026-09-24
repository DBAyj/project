#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
"$root/scripts/test_p3.sh"
make -C "$root" docs-verify
"$root/scripts/verify_p1.sh"
"$root/scripts/verify_p2.sh"
"$root/scripts/run_p3.sh"
trap '"$root/scripts/stop_p3.sh" >/dev/null 2>&1 || true' EXIT
"$root/.venv/bin/python" "$root/tests/p3-e2e/test_p3_live_service.py" --root "$root" --socket "$root/runtime/spatial/sockets/astra-spatial.sock" --token "$(cat "$root/runtime/state/p3-capability.token")" --output "$root/runtime/tmp/p3-live-acceptance.json"
"$root/scripts/stop_p3.sh"
trap - EXIT
"$root/.venv/bin/python" "$root/scripts/verify_p3_baseline.py"
echo "P3 verification passed"
