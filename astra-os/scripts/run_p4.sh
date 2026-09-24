#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
socket="$root/runtime/state/p4-projection.sock"
service_pid_file="$root/runtime/state/p4-projection-service.pid"
shell_pid_file="$root/runtime/state/p4-shell.pid"
token_file="$root/runtime/state/p4-projection.token"
"$root/scripts/stop_p4.sh" || true
"$root/scripts/build_p4.sh" >/dev/null
mkdir -p "$root/runtime/logs" "$root/runtime/state" "$root/runtime/audit"
# The service rejects capability tokens shorter than 32 characters; use a fresh per-run token.
"$root/.venv/bin/python" -c 'import secrets; print(secrets.token_urlsafe(48))' > "$token_file"
chmod 600 "$token_file"
cd "$root"
nohup "$root/runtime/tmp/p4-build/services/astra-projection-service/astra-projection-service" --socket "$socket" \
  --capability-token-file "$token_file" > "$root/runtime/logs/p4-projection-service.log" 2>&1 < /dev/null &
echo $! > "$service_pid_file"
for attempt in {1..30}; do
  if "$root/.venv/bin/python" "$root/scripts/p4_service_client.py" --socket "$socket" --token-file "$token_file" --health >/dev/null 2>&1; then break; fi
  if (( attempt == 30 )); then
    echo "P4 service did not become ready" >&2
    exit 1
  fi
  sleep 0.1
done
"$root/.venv/bin/python" "$root/scripts/p4_service_client.py" --socket "$socket" --token-file "$token_file" --health >/dev/null
nohup env QSG_RHI_BACKEND=metal ASTRA_PROJECT_ROOT="$root" ASTRA_P4_PROJECTION_SOCKET="$socket" ASTRA_P4_PROJECTION_TOKEN_FILE="$token_file" ASTRA_P4_DEMO_ON_START=1 \
  "$root/runtime/tmp/p4-build/apps/astra-shell/astra-shell" > "$root/runtime/logs/p4-shell.log" 2>&1 < /dev/null &
echo $! > "$shell_pid_file"
echo "P4 service PID: $(cat "$service_pid_file")"
echo "P4 shell PID: $(cat "$shell_pid_file")"
