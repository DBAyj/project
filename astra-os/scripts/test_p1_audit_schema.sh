#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
evidence="$root/runtime/tmp/p1-audit/p1-1-audit-schema.jsonl"
rm -f "$evidence"
"$root/scripts/build_p1.sh" >/dev/null
ASTRA_AUDIT_EVIDENCE_PATH="$evidence" \
  ctest --test-dir "$root/runtime/tmp/p1-build" -R '^astra-shell-controller-integration-tests$' --output-on-failure
"$root/.venv/bin/python" "$root/scripts/verify_p1_audit_schema.py" "$evidence"
