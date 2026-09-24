#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
state="$root/runtime/state"
if [[ "$(uname -s)" == "Darwin" ]]; then
  launchctl remove dev.astraos.p3.shell 2>/dev/null || true
  launchctl remove dev.astraos.p3.spatial 2>/dev/null || true
fi
for name in p3-shell p3-spatial-service; do
  pid_file="$state/$name.pid"
  [[ -s "$pid_file" ]] || continue
  pid="$(cat "$pid_file")"
  command="$(ps -p "$pid" -o command= 2>/dev/null || true)"
  if [[ -n "$command" && "$command" == *"$root"* ]]; then kill -TERM "$pid" 2>/dev/null || true; fi
  rm -f "$pid_file"
done
rm -f "$state/p3-capability.token" "$root/runtime/spatial/sockets/astra-spatial.sock"
"$root/scripts/stop_p2.sh" >/dev/null 2>&1 || true
echo "P3 stopped"
