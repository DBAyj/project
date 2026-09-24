#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
"$root/scripts/build_p5.sh" >/dev/null
mkdir -p "$root/runtime/reports"
"$root/runtime/tmp/p5-build/libraries/astra-ui/astra-ui-p5-performance-benchmark" \
  "$root/runtime/reports/p5-performance.json"
python3 "$root/scripts/write_p5_performance_report.py" \
  "$root/runtime/reports/p5-performance.json" "$root/docs/reports/p5-performance-report.md"
