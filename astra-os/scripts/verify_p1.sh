#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
"$root/scripts/test_p1.sh"
file "$root/runtime/tmp/p1-build/apps/astra-shell/astra-shell" | grep -q arm64
test -s "$root/protocols/intent/intent-v1.schema.json"
test -s "$root/protocols/projection/projection-session-v1.schema.json"
test -s "$root/protocols/audit/audit-event-v1.schema.json"
echo "P1 verification passed"
