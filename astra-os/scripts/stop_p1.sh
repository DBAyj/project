#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
pid_file="$root/runtime/state/p1-shell.pid"
[[ -f "$pid_file" ]] || exit 0
pid="$(cat "$pid_file")"
if kill -0 "$pid" 2>/dev/null; then kill "$pid"; fi
rm -f "$pid_file"
