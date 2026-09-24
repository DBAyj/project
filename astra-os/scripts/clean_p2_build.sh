#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
for pid_file in "$root/runtime/state/p2-intent-service.pid" "$root/runtime/state/p2-shell.pid"; do
  if [[ -s "$pid_file" ]] && kill -0 "$(cat "$pid_file")" 2>/dev/null; then
    echo "Stop P2 before cleaning generated state" >&2
    exit 1
  fi
done
rm -rf "$root/runtime/tmp/p2-tests" "$root/runtime/tmp/p2-performance.json" "$root/runtime/tmp/p2-stability.json" \
  "$root/runtime/tmp/p2-live-acceptance.json" "$root/runtime/tmp/p2-verification-summary.json"
rm -f "$root/runtime/state/p2-intent-service.pid" "$root/runtime/state/p2-shell.pid" \
  "$root/runtime/state/p2-capability.token" "$root/runtime/intent/sockets/astra-intent.sock"
echo "P2 generated build and test state cleaned"
