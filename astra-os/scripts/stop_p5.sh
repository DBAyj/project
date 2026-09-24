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
credential_dir="$root/runtime/spatial-ui/credentials"
client_token_file="$credential_dir/client.token"
supervisor_token_file="$credential_dir/supervisor.token"
projection_token_file="$credential_dir/projection.token"
spatial_target_token_file="$credential_dir/spatial.token"

if [[ -f "$shell_pid_file" ]]; then
  pid="$(cat "$shell_pid_file")"
  if kill -0 "$pid" 2>/dev/null; then kill "$pid" || true; fi
  rm -f "$shell_pid_file"
fi
if [[ -S "$spatial_socket" && -f "$client_token_file" && -f "$supervisor_token_file" ]]; then
  "$root/.venv/bin/python" "$root/scripts/p5_service_client.py" --socket "$spatial_socket" --token-file "$client_token_file" \
    --supervisor-token-file "$supervisor_token_file" --shutdown >/dev/null 2>&1 || true
fi
if [[ -S "$projection_socket" && -f "$projection_token_file" ]]; then
  "$root/.venv/bin/python" "$root/scripts/p4_service_client.py" --socket "$projection_socket" --token-file "$projection_token_file" --stop >/dev/null 2>&1 || true
fi
for pid_file in "$spatial_pid_file" "$projection_pid_file" "$spatial_target_pid_file"; do
  [[ -f "$pid_file" ]] || continue
  pid="$(cat "$pid_file")"
  attempts=0
  while kill -0 "$pid" 2>/dev/null && (( attempts < 30 )); do
    sleep 0.05
    ((attempts += 1))
  done
  if kill -0 "$pid" 2>/dev/null; then kill "$pid" || true; fi
  rm -f "$pid_file"
done
rm -f "$spatial_socket" "$projection_socket" "$spatial_target_socket" "$root/runtime/state/p5-http-port" "$root/runtime/cache/p4-output.png"
rm -f "$client_token_file" "$supervisor_token_file" "$projection_token_file" "$spatial_target_token_file"
rmdir "$credential_dir" 2>/dev/null || true
