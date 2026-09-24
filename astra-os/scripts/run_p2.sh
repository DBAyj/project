#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
state="$root/runtime/state"
logs="$root/runtime/logs"
service_pid_file="$state/p2-intent-service.pid"
shell_pid_file="$state/p2-shell.pid"
token_file="$state/p2-capability.token"
socket_path="$root/runtime/intent/sockets/astra-intent.sock"
for pid_file in "$service_pid_file" "$shell_pid_file"; do
  if [[ -s "$pid_file" ]] && kill -0 "$(cat "$pid_file")" 2>/dev/null; then
    echo "P2 is already running with PID $(cat "$pid_file")" >&2
    exit 1
  fi
done
"$root/scripts/build_p2.sh" >/dev/null
interface="$(route -n get default 2>/dev/null | awk '/interface:/{print $2; exit}')"
http_host="$(ipconfig getifaddr "$interface" 2>/dev/null || true)"
if [[ -z "$http_host" || "$http_host" == "127.0.0.1" ]]; then
  echo "No active non-loopback IPv4 address was found" >&2
  exit 1
fi
mkdir -p "$state" "$logs" "$(dirname "$socket_path")"
"$root/.venv/bin/python" -c 'import secrets; print(secrets.token_hex(32))' >"$token_file"
chmod 600 "$token_file"
token="$(cat "$token_file")"
if [[ "$(uname -s)" == "Darwin" ]]; then
  launchctl remove dev.astraos.p2.intent 2>/dev/null || true
  launchctl remove dev.astraos.p2.shell 2>/dev/null || true
  launchctl submit -l dev.astraos.p2.intent -o "$logs/p2-intent-service.log" -e "$logs/p2-intent-service.log" -- \
    /usr/bin/env ASTRA_INTENT_CAPABILITY_TOKEN="$token" PYTHONUNBUFFERED=1 \
    "$root/.venv/bin/python" -m astra_intent.main --root "$root" --socket "$socket_path" --http-host "$http_host"
  service_pid="$(launchctl print "gui/$(id -u)/dev.astraos.p2.intent" | awk '/pid =/{print $3; exit}')"
else
  ASTRA_INTENT_CAPABILITY_TOKEN="$token" PYTHONUNBUFFERED=1 \
    nohup "$root/.venv/bin/python" -m astra_intent.main --root "$root" --socket "$socket_path" \
      --http-host "$http_host" </dev/null >"$logs/p2-intent-service.log" 2>&1 &
  service_pid=$!
fi
echo "$service_pid" >"$service_pid_file"
ready=0
for _ in $(seq 1 50); do
  if [[ -S "$socket_path" ]] && curl --noproxy '*' --silent --fail --max-time 1 \
      -H "X-Astra-Capability: $token" "http://$http_host:8765/health" >/dev/null; then
    ready=1
    break
  fi
  sleep 0.1
done
if [[ "$ready" -ne 1 ]]; then
  "$root/scripts/stop_p2.sh" || true
  echo "Intent Service failed readiness; see $logs/p2-intent-service.log" >&2
  exit 1
fi
if [[ "$(uname -s)" == "Darwin" ]]; then
  launchctl submit -l dev.astraos.p2.shell -o "$logs/p2-shell.log" -e "$logs/p2-shell.log" -- \
    /usr/bin/env ASTRA_P2_MODE=1 ASTRA_INTENT_SOCKET_PATH="$socket_path" ASTRA_INTENT_CAPABILITY_TOKEN="$token" \
    QSG_RHI_BACKEND=metal QSG_INFO=1 "$root/runtime/tmp/p1-build/apps/astra-shell/astra-shell"
  shell_pid="$(launchctl print "gui/$(id -u)/dev.astraos.p2.shell" | awk '/pid =/{print $3; exit}')"
else
  ASTRA_P2_MODE=1 ASTRA_INTENT_SOCKET_PATH="$socket_path" ASTRA_INTENT_CAPABILITY_TOKEN="$token" \
  QSG_RHI_BACKEND=metal QSG_INFO=1 \
    nohup "$root/runtime/tmp/p1-build/apps/astra-shell/astra-shell" </dev/null >"$logs/p2-shell.log" 2>&1 &
  shell_pid=$!
fi
echo "$shell_pid" >"$shell_pid_file"
sleep 1
if ! kill -0 "$shell_pid" 2>/dev/null; then
  "$root/scripts/stop_p2.sh" || true
  echo "Astra Shell failed startup; see $logs/p2-shell.log" >&2
  exit 1
fi
echo "P2 running service_pid=$service_pid shell_pid=$shell_pid socket=$socket_path http=http://$http_host:8765"
