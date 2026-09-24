#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
"$root/scripts/build_p5.sh" >/dev/null
mkdir -p "$root/runtime/reports"
"$root/runtime/tmp/p5-build/libraries/astra-ui/astra-ui-p5-stability-test" \
  "$root/runtime/reports/p5-stability.json"
ASTRA_P5_INTEGRATION_CYCLES=100 \
  "$root/runtime/tmp/p5-build/apps/astra-shell/astra-shell-p5-spatial-ui-integration-tests"
python3 - "$root/runtime/reports/p5-stability.json" <<'PY'
import json
import sys
from pathlib import Path

path = Path(sys.argv[1])
metrics = json.loads(path.read_text(encoding="utf-8"))
metrics["real_service_chain_cycles"] = 100
path.write_text(json.dumps(metrics, indent=2, sort_keys=True) + "\n", encoding="utf-8")
PY
python3 "$root/scripts/write_p5_stability_report.py" \
  "$root/runtime/reports/p5-stability.json" "$root/docs/reports/p5-stability-report.md"
