#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
python3 "$root/tools/projection-fixture-generator/generate.py" --output "$root/assets/projection-fixtures"
