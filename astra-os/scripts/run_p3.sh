#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
state="$root/runtime/state"
logs="$root/runtime/logs"
socket="$root/runtime/spatial/sockets/astra-spatial.sock"
"$root/scripts/stop_p3.sh" >/dev/null 2>&1 || true
"$root/scripts/build_p3.sh" >/dev/null
"$root/scripts/generate_spatial_fixtures.sh"
"$root/scripts/run_p2.sh" >/dev/null
if [[ "$(uname -s)" == "Darwin" ]]; then
  launchctl remove dev.astraos.p2.shell 2>/dev/null || true
fi
rm -f "$state/p2-shell.pid"
mkdir -p "$state" "$logs" "$(dirname "$socket")"
token="$("$root/.venv/bin/python" -c 'import secrets; print(secrets.token_hex(32))')"
printf '%s\n' "$token" >"$state/p3-capability.token"
chmod 600 "$state/p3-capability.token"
service="$root/runtime/tmp/p1-build/services/astra-spatial-service/astra-spatial-service"
shell="$root/runtime/tmp/p1-build/apps/astra-shell/astra-shell"
if [[ "$(uname -s)" == "Darwin" ]]; then
  launchctl submit -l dev.astraos.p3.spatial -o "$logs/p3-spatial-service.log" -e "$logs/p3-spatial-service.log" -- \
    /usr/bin/env ASTRA_SPATIAL_CAPABILITY_TOKEN="$token" "$service" --root "$root" --socket "$socket" --token "$token" --capabilities spatial.read,spatial.control,spatial.observer.write
  service_pid="$(launchctl print "gui/$(id -u)/dev.astraos.p3.spatial" | awk '/pid =/{print $3; exit}')"
  launchctl submit -l dev.astraos.p3.shell -o "$logs/p3-shell.log" -e "$logs/p3-shell.log" -- \
    /usr/bin/env ASTRA_P2_MODE=1 ASTRA_P3_MODE=1 ASTRA_INTENT_SOCKET_PATH="$root/runtime/intent/sockets/astra-intent.sock" ASTRA_INTENT_CAPABILITY_TOKEN="$(cat "$state/p2-capability.token")" ASTRA_SPATIAL_SOCKET_PATH="$socket" ASTRA_SPATIAL_CAPABILITY_TOKEN="$token" QSG_RHI_BACKEND=metal "$shell"
  shell_pid="$(launchctl print "gui/$(id -u)/dev.astraos.p3.shell" | awk '/pid =/{print $3; exit}')"
else
  ASTRA_SPATIAL_CAPABILITY_TOKEN="$token" nohup "$service" --root "$root" --socket "$socket" --token "$token" --capabilities spatial.read,spatial.control,spatial.observer.write >"$logs/p3-spatial-service.log" 2>&1 &
  service_pid=$!
  ASTRA_P2_MODE=1 ASTRA_P3_MODE=1 ASTRA_INTENT_SOCKET_PATH="$root/runtime/intent/sockets/astra-intent.sock" ASTRA_INTENT_CAPABILITY_TOKEN="$(cat "$state/p2-capability.token")" ASTRA_SPATIAL_SOCKET_PATH="$socket" ASTRA_SPATIAL_CAPABILITY_TOKEN="$token" nohup "$shell" >"$logs/p3-shell.log" 2>&1 &
  shell_pid=$!
fi
printf '%s\n' "$service_pid" >"$state/p3-spatial-service.pid"
printf '%s\n' "$shell_pid" >"$state/p3-shell.pid"
for _ in $(seq 1 50); do
  [[ -S "$socket" ]] && break
  sleep 0.1
done
[[ -S "$socket" ]]
echo "P3 running service_pid=$service_pid shell_pid=$shell_pid socket=$socket"
