#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
"$root/scripts/stop_p1.sh" || true
trap '"$root/scripts/stop_p1.sh"' EXIT
QSG_INFO=1 "$root/scripts/run_p1.sh"
sleep 2
(
  cd "$root"
  python3 scripts/verify_p1_graphics_backend.py
)
