#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
report="$root/docs/reports/p5-release-gate-report.md"
"$root/scripts/stop_p5.sh" || true
trap '"$root/scripts/stop_p5.sh"' EXIT

make -C "$root" format
make -C "$root" lint
make -C "$root" p1-test
make -C "$root" p1-verify

make -C "$root" p2-test
make -C "$root" p2-verify
make -C "$root" p3-test
make -C "$root" p3-verify
make -C "$root" p4-test
make -C "$root" p4-verify

make -C "$root" p5-generate-fixtures
make -C "$root" p5-configure
make -C "$root" p5-build
make -C "$root" p5-test
make -C "$root" p5-test-unit
make -C "$root" p5-test-contract
make -C "$root" p5-test-qml
make -C "$root" p5-test-integration
make -C "$root" p5-test-accessibility
make -C "$root" p5-test-graphics
make -C "$root" p5-test-security
make -C "$root" p5-test-performance
make -C "$root" p5-test-stability
make -C "$root" p5-verify
make -C "$root" docs-verify

"$root/scripts/run_p5.sh"
sleep 2
port="$(cat "$root/runtime/state/p5-http-port")"
client_token_file="$root/runtime/spatial-ui/credentials/client.token"
client_token="$(cat "$client_token_file")"
http_read_token="$(python3 -c 'import hashlib,sys; print(hashlib.sha256((sys.argv[1] + ":spatial_ui.read").encode()).hexdigest())' "$client_token")"
curl --fail --silent -H "Authorization: Bearer $http_read_token" \
  "http://127.0.0.1:$port/v1/spatial-ui/status" | grep -q 'P4_RELEASE_BASELINE_FINAL'
"$root/.venv/bin/python" "$root/scripts/p5_service_client.py" \
  --socket "$root/runtime/spatial-ui/sockets/astra-spatial-ui.sock" \
  --token-file "$client_token_file" --status | grep -q '"status": "READY"'
"$root/.venv/bin/python" "$root/scripts/p3_target_client.py" \
  --socket "$root/runtime/state/p5-spatial.sock" \
  --token-file "$root/runtime/spatial-ui/credentials/spatial.token" --state \
  > "$root/runtime/reports/p5-release-spatial-target.json"
"$root/.venv/bin/python" "$root/scripts/p4_service_client.py" \
  --socket "$root/runtime/state/p5-projection.sock" \
  --token-file "$root/runtime/spatial-ui/credentials/projection.token" --frame \
  > "$root/runtime/reports/p5-release-projection-frame.json"
"$root/.venv/bin/python" - "$root/runtime/reports/p5-release-spatial-target.json" \
  "$root/runtime/reports/p5-release-projection-frame.json" <<'PY'
import json
import sys
import uuid
from pathlib import Path

spatial = json.loads(Path(sys.argv[1]).read_text(encoding="utf-8"))
frame = json.loads(Path(sys.argv[2]).read_text(encoding="utf-8"))
target = spatial.get("target", {})
target_id = target.get("target_id", "")
uuid.UUID(target_id)
if spatial.get("state") != "TRACKING" or target.get("state") != "CALIBRATED" or not target.get("selected"):
    raise SystemExit("P3 target is not TRACKING/CALIBRATED/selected")
if frame.get("p3_integration_status") != "P3_SERVICE_PROJECTION_TARGET_VERIFIED":
    raise SystemExit("P4 frame is not bound to a verified P3 service target")
if frame.get("spatial_target_id") != target_id:
    raise SystemExit("P3 and P4 target identities do not match")
PY
"$root/scripts/stop_p5.sh"
for residue in \
  "$root/runtime/state/p5-spatial-ui-service.pid" \
  "$root/runtime/state/p5-projection-service.pid" \
  "$root/runtime/state/p5-p3-spatial-service.pid" \
  "$root/runtime/state/p5-shell.pid" \
  "$root/runtime/state/p5-spatial.sock" \
  "$root/runtime/state/p5-projection.sock" \
  "$root/runtime/spatial-ui/sockets/astra-spatial-ui.sock" \
  "$root/runtime/spatial-ui/credentials/client.token" \
  "$root/runtime/spatial-ui/credentials/supervisor.token" \
  "$root/runtime/spatial-ui/credentials/projection.token" \
  "$root/runtime/spatial-ui/credentials/spatial.token"; do
  test ! -e "$residue"
done

cat > "$report" <<EOF
# P5 Release Gate Report

Baseline: \`P4_RELEASE_BASELINE_FINAL\`

Result: \`P5_RELEASE_GATE_PASSED\`

P1-P4 regressions, P5 fixture generation, ARM64 build, unit, per-method
contract, QML, three-service integration, accessibility, real Metal graphics,
privacy/security, performance, stability, and the final project-owned run/stop
checks completed. The live run reached P3 TRACKING with a calibrated UUID
ProjectionTarget, propagated that same identity through P5 into the P4 output
frame, reached authenticated READY states, and stopped without PID, Socket, or
token residue. The formal P1-P4 version/tag ancestry also passed.

No privacy leak, input safety-layer penetration, stale destroyed focus, target-
loss sensitive residue, P1/P4 regression, or core accessibility failure was
downgraded to a warning.
EOF
echo "P5_RELEASE_GATE_PASSED"
