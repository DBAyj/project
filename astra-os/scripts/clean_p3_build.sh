#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
for pid_file in "$root/runtime/state/p3-spatial-service.pid" "$root/runtime/state/p3-shell.pid"; do
  [[ ! -s "$pid_file" ]] || { echo "Stop P3 before cleaning generated state" >&2; exit 1; }
done
rm -rf "$root/runtime/tmp/p3-tests" "$root/runtime/tmp/p3-performance.json" "$root/runtime/tmp/p3-stability.json" "$root/runtime/tmp/p3-live-acceptance.json"
rm -f "$root/runtime/spatial/sockets/astra-spatial.sock" "$root/runtime/state/p3-capability.token"
echo "P3 generated state cleaned"
