#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
"$root/scripts/stop_p4.sh" || true
rm -rf "$root/runtime/tmp/p4-build"
rm -f "$root/runtime/cache/p4-output.png"
rm -f "$root/runtime/reports/p4-performance.json"
rm -f "$root/runtime/logs/p4-shell.log" "$root/runtime/logs/p4-projection-service.log"
rm -f "$root/runtime/state/p4-projection.sock" "$root/runtime/state/p4-projection-service.pid" "$root/runtime/state/p4-shell.pid"
