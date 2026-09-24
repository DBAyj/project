#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
socket="$root/runtime/state/p4-projection.sock"
"$root/scripts/stop_p4.sh" || true
trap '"$root/scripts/stop_p4.sh"' EXIT
"$root/scripts/run_p4.sh"
sleep 1
"$root/.venv/bin/python" "$root/scripts/p4_service_client.py" --socket "$socket" --health
ctest --test-dir "$root/runtime/tmp/p4-build" -R '^astra-shell-p4-projection-integration-tests$' --output-on-failure
"$root/.venv/bin/python" "$root/scripts/p4_service_client.py" --socket "$socket" --stop
