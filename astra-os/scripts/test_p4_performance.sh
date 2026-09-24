#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
socket="$root/runtime/state/p4-projection.sock"
token_file="$root/runtime/state/p4-projection.token"
"$root/scripts/stop_p4.sh" || true
trap '"$root/scripts/stop_p4.sh"' EXIT
"$root/scripts/run_p4.sh"
sleep 1
"$root/.venv/bin/python" "$root/scripts/p4_service_client.py" --socket "$socket" --token-file "$token_file" --benchmark 120 --report "$root/runtime/reports/p4-performance.json"
"$root/.venv/bin/python" "$root/scripts/verify_p4_baseline.py" "$root/runtime/reports/p4-performance.json" "$root/docs/reports/p4-performance-report.md"
