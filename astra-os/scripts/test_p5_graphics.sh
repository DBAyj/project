#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
"$root/scripts/stop_p5.sh" || true
trap '"$root/scripts/stop_p5.sh"' EXIT
ASTRA_P5_GRAPHICS_PROBE=1 "$root/scripts/run_p5.sh"
sleep 3
python3 "$root/scripts/verify_p5_graphics.py"
"$root/scripts/stop_p5.sh"
test ! -e "$root/runtime/state/p5-spatial-ui-service.pid"
test ! -e "$root/runtime/state/p5-shell.pid"
test ! -e "$root/runtime/spatial-ui/sockets/astra-spatial-ui.sock"
