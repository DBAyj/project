#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
python3 "$root/tools/spatial-ui-fixture-generator/generate.py" --output "$root/assets/spatial-ui-fixtures"
