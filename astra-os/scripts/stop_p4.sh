#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
socket="$root/runtime/state/p4-projection.sock"
service_pid_file="$root/runtime/state/p4-projection-service.pid"
shell_pid_file="$root/runtime/state/p4-shell.pid"
if [[ -S "$socket" ]]; then "$root/.venv/bin/python" "$root/scripts/p4_service_client.py" --socket "$socket" --stop >/dev/null 2>&1 || true; fi
for pid_file in "$shell_pid_file" "$service_pid_file"; do
  [[ -f "$pid_file" ]] || continue
  pid="$(cat "$pid_file")"
  if kill -0 "$pid" 2>/dev/null; then kill "$pid" || true; fi
  rm -f "$pid_file"
done
rm -f "$socket"
rm -f "$root/runtime/cache/p4-output.png"
