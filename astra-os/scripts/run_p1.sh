#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
pid_file="$root/runtime/state/p1-shell.pid"
mkdir -p "$root/runtime/logs" "$root/runtime/state" "$root/runtime/audit"
if [[ -f "$pid_file" ]] && kill -0 "$(cat "$pid_file")" 2>/dev/null; then echo "P1 shell is already running" >&2; exit 1; fi
"$root/scripts/build_p1.sh" >/dev/null
nohup env QSG_RHI_BACKEND=metal "$root/runtime/tmp/p1-build/apps/astra-shell/astra-shell" \
  > "$root/runtime/logs/p1-shell.log" 2>&1 < /dev/null &
echo $! > "$pid_file"
echo "P1 shell PID: $(cat "$pid_file")"
