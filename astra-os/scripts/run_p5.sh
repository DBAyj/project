#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
spatial_socket="$root/runtime/spatial-ui/sockets/astra-spatial-ui.sock"
projection_socket="$root/runtime/state/p5-projection.sock"
spatial_target_socket="$root/runtime/state/p5-spatial.sock"
spatial_pid_file="$root/runtime/state/p5-spatial-ui-service.pid"
projection_pid_file="$root/runtime/state/p5-projection-service.pid"
spatial_target_pid_file="$root/runtime/state/p5-p3-spatial-service.pid"
shell_pid_file="$root/runtime/state/p5-shell.pid"
http_port_file="$root/runtime/state/p5-http-port"
credential_dir="$root/runtime/spatial-ui/credentials"
client_token_file="$credential_dir/client.token"
supervisor_token_file="$credential_dir/supervisor.token"
projection_token_file="$credential_dir/projection.token"
spatial_target_token_file="$credential_dir/spatial.token"
"$root/scripts/stop_p5.sh" || true
trap '"$root/scripts/stop_p5.sh" >/dev/null 2>&1 || true' ERR
"$root/scripts/build_p5.sh" >/dev/null
mkdir -p "$root/runtime/logs" "$root/runtime/state" "$root/runtime/audit" "$root/runtime/reports" \
  "$root/runtime/spatial-ui/sockets" "$root/runtime/spatial-ui/state" "$root/runtime/spatial-ui/audit" "$credential_dir"
umask 077
"$root/.venv/bin/python" -c 'import secrets,sys; print(secrets.token_urlsafe(48))' > "$client_token_file"
"$root/.venv/bin/python" -c 'import secrets,sys; print(secrets.token_urlsafe(48))' > "$supervisor_token_file"
"$root/.venv/bin/python" -c 'import secrets,sys; print(secrets.token_urlsafe(48))' > "$projection_token_file"
"$root/.venv/bin/python" -c 'import secrets,sys; print(secrets.token_urlsafe(48))' > "$spatial_target_token_file"

nohup "$root/runtime/tmp/p5-build/services/astra-spatial-service/astra-spatial-service" \
  --root "$root" --socket "$spatial_target_socket" \
  --token "$(cat "$spatial_target_token_file")" --capabilities spatial.read,spatial.control \
  --fixture "$root/assets/spatial-fixtures/desk-front.png" \
  > "$root/runtime/logs/p5-p3-spatial-service.log" 2>&1 < /dev/null &
echo $! > "$spatial_target_pid_file"
for attempt in {1..40}; do
  if "$root/.venv/bin/python" "$root/scripts/p3_target_client.py" --socket "$spatial_target_socket" \
      --token-file "$spatial_target_token_file" --health >/dev/null 2>&1; then break; fi
  if (( attempt == 40 )); then echo "P3 spatial target service did not become ready" >&2; exit 1; fi
  sleep 0.1
done
"$root/.venv/bin/python" "$root/scripts/p3_target_client.py" --socket "$spatial_target_socket" \
  --token-file "$spatial_target_token_file" --calibrate > "$root/runtime/reports/p5-live-spatial-target.json"

http_port="$("$root/.venv/bin/python" -c 'import socket; s=socket.socket(); s.bind(("127.0.0.1", 0)); print(s.getsockname()[1]); s.close()')"
echo "$http_port" > "$http_port_file"

nohup "$root/runtime/tmp/p5-build/services/astra-projection-service/astra-projection-service" --socket "$projection_socket" \
  --capability-token-file "$projection_token_file" \
  > "$root/runtime/logs/p5-projection-service.log" 2>&1 < /dev/null &
echo $! > "$projection_pid_file"
for attempt in {1..40}; do
  if "$root/.venv/bin/python" "$root/scripts/p4_service_client.py" --socket "$projection_socket" --token-file "$projection_token_file" --health >/dev/null 2>&1; then break; fi
  if (( attempt == 40 )); then echo "P4 projection fixture service did not become ready" >&2; exit 1; fi
  sleep 0.1
done

nohup "$root/runtime/tmp/p5-build/services/astra-spatial-ui-service/astra-spatial-ui-service" \
  --project-root "$root" \
  --socket "$spatial_socket" --state "$root/runtime/spatial-ui/state/ui-state.json" \
  --audit "$root/runtime/spatial-ui/audit/spatial-ui-audit.jsonl" \
  --client-token-file "$client_token_file" --supervisor-token-file "$supervisor_token_file" \
  --projection-socket "$projection_socket" --projection-token-file "$projection_token_file" \
  --spatial-socket "$spatial_target_socket" --spatial-token-file "$spatial_target_token_file" \
  --http-address 127.0.0.1 --http-port "$http_port" \
  > "$root/runtime/logs/p5-spatial-ui-service.log" 2>&1 < /dev/null &
echo $! > "$spatial_pid_file"
for attempt in {1..40}; do
  if "$root/.venv/bin/python" "$root/scripts/p5_service_client.py" --socket "$spatial_socket" --token-file "$client_token_file" --health >/dev/null 2>&1; then break; fi
  if (( attempt == 40 )); then echo "P5 Spatial UI service did not become ready" >&2; exit 1; fi
  sleep 0.1
done
curl --fail --silent "http://127.0.0.1:$http_port/health" >/dev/null
public_component_json="$("$root/.venv/bin/python" "$root/scripts/p5_service_client.py" --socket "$spatial_socket" --token-file "$client_token_file" --create-public)"
public_component_id="$(python3 -c 'import json, sys; print(json.load(sys.stdin)["component_id"])' <<< "$public_component_json")"
"$root/.venv/bin/python" "$root/scripts/p5_service_client.py" --socket "$spatial_socket" --token-file "$client_token_file" --create-private >/dev/null
"$root/.venv/bin/python" "$root/scripts/p5_service_client.py" --socket "$spatial_socket" --token-file "$client_token_file" --focus "$public_component_id" >/dev/null
"$root/.venv/bin/python" "$root/scripts/p5_service_client.py" --socket "$spatial_socket" --token-file "$client_token_file" \
  --open-fixture-window "$public_component_id" >/dev/null
"$root/.venv/bin/python" "$root/scripts/p5_service_client.py" --socket "$spatial_socket" --token-file "$client_token_file" --move-fixture-window >/dev/null
"$root/.venv/bin/python" "$root/scripts/p5_service_client.py" --socket "$spatial_socket" --token-file "$client_token_file" --resize-fixture-window >/dev/null
"$root/.venv/bin/python" "$root/scripts/p4_service_client.py" --socket "$projection_socket" \
  --token-file "$projection_token_file" --frame > "$root/runtime/reports/p5-live-projection-frame.json"
"$root/.venv/bin/python" - "$root/runtime/reports/p5-live-projection-frame.json" <<'PY'
import json
import sys
import uuid
from pathlib import Path

frame = json.loads(Path(sys.argv[1]).read_text(encoding="utf-8"))
if frame.get("p3_integration_status") != "P3_SERVICE_PROJECTION_TARGET_VERIFIED":
    raise SystemExit("P4 frame is not bound to a verified P3 service target")
uuid.UUID(frame["spatial_target_id"])
PY

nohup env QSG_RHI_BACKEND=metal ASTRA_PROJECT_ROOT="$root" \
  ASTRA_P4_PROJECTION_SOCKET="$projection_socket" ASTRA_P4_PROJECTION_TOKEN_FILE="$projection_token_file" \
  ASTRA_P5_SPATIAL_UI_SOCKET="$spatial_socket" ASTRA_P5_SPATIAL_UI_TOKEN_FILE="$client_token_file" \
  "$root/runtime/tmp/p5-build/apps/astra-shell/astra-shell" > "$root/runtime/logs/p5-shell.log" 2>&1 < /dev/null &
echo $! > "$shell_pid_file"

echo "P1 shell fallback: available"
echo "P2 status: fixture adapter only"
echo "P3 spatial service: simulated target calibrated and verified"
echo "P4 projection service: verified P3 target frame ready"
echo "Spatial UI service PID: $(cat "$spatial_pid_file")"
echo "Astra Shell PID: $(cat "$shell_pid_file")"
echo "Socket path: $spatial_socket"
echo "Development HTTP: http://127.0.0.1:$http_port"
echo "Graphics backend: Metal (QSG_RHI_BACKEND=metal)"
echo "Fixture interactions: privacy split, focus, window open/move/resize accepted"
echo "P4_RELEASE_BASELINE_FINAL"
trap - ERR
