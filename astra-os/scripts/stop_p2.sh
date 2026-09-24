#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
state="$root/runtime/state"
if [[ "$(uname -s)" == "Darwin" ]]; then
  launchctl remove dev.astraos.p2.shell 2>/dev/null || true
  launchctl remove dev.astraos.p2.intent 2>/dev/null || true
fi
for name in p2-shell p2-intent-service; do
  pid_file="$state/$name.pid"
  [[ -s "$pid_file" ]] || continue
  pid="$(cat "$pid_file")"
  command="$(ps -p "$pid" -o command= 2>/dev/null || true)"
  if [[ -n "$command" && "$command" == *"$root"* ]]; then
    kill -TERM "$pid" 2>/dev/null || true
    for _ in $(seq 1 50); do
      kill -0 "$pid" 2>/dev/null || break
      sleep 0.1
    done
    if kill -0 "$pid" 2>/dev/null; then
      kill -KILL "$pid" 2>/dev/null || true
    fi
  fi
  rm -f "$pid_file"
done
rm -f "$state/p2-capability.token"
if [[ -S "$root/runtime/intent/sockets/astra-intent.sock" ]]; then
  rm -f "$root/runtime/intent/sockets/astra-intent.sock"
fi
echo "P2 stopped"
