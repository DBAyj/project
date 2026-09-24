#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
python_bin="$(command -v python3.12 || true)"
if [[ -z "$python_bin" ]]; then
  echo "Python 3.12 is required" >&2
  exit 1
fi
if [[ ! -x "$root/.venv/bin/python" ]] || [[ "$("$root/.venv/bin/python" -c 'import sys; print(f"{sys.version_info.major}.{sys.version_info.minor}")')" != "3.12" ]]; then
  "$python_bin" -m venv "$root/.venv"
fi
"$root/.venv/bin/python" -m pip install -e "$root/services/astra-intent-service"
mkdir -p "$root/runtime/intent/cache" "$root/runtime/intent/state" "$root/runtime/intent/sockets" \
  "$root/runtime/logs" "$root/runtime/state" "$root/runtime/tmp"
(cd "$root" && "$root/.venv/bin/python" -c 'from pathlib import Path; from astra_intent.infrastructure.configuration import IntentConfiguration; IntentConfiguration.load(Path.cwd())')
echo "P2 configured python=$("$root/.venv/bin/python" --version 2>&1)"
