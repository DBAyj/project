#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
"$root/.venv/bin/python" "$root/tests/contract/test_p5_schemas.py"
