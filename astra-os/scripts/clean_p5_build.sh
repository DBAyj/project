#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
"$root/scripts/stop_p5.sh" || true
rm -rf "$root/runtime/tmp/p5-build"
