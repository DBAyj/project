#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
python3.12 "$root/tools/spatial-fixture-generator/generate_fixtures.py" \
  --output "$root/assets/spatial-fixtures"
