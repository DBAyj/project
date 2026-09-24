from __future__ import annotations

import argparse
import json
from pathlib import Path
import socket
import uuid


def call(socket_path: Path, token: str, method: str, params: dict[str, object]) -> dict[str, object]:
    request = {
        "jsonrpc": "2.0",
        "id": str(uuid.uuid4()),
        "trace_id": str(uuid.uuid4()),
        "method": method,
        "params": params,
        "security_context": {"capability_token": token},
    }
    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as client:
        client.settimeout(2.0)
        client.connect(str(socket_path))
        client.sendall(json.dumps(request, separators=(",", ":")).encode("utf-8") + b"\n")
        response = b""
        while not response.endswith(b"\n"):
            response += client.recv(65536)
    value = json.loads(response)
    if "error" in value:
        raise AssertionError(value["error"])
    return value["result"]


def subscribe(socket_path: Path, token: str) -> socket.socket:
    client = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    client.settimeout(2.0)
    client.connect(str(socket_path))
    request = {
        "jsonrpc": "2.0",
        "id": str(uuid.uuid4()),
        "trace_id": str(uuid.uuid4()),
        "method": "event.subscribe",
        "params": {"event": "spatial.state_changed"},
        "security_context": {"capability_token": token},
    }
    client.sendall(json.dumps(request, separators=(",", ":")).encode("utf-8") + b"\n")
    response = b""
    while not response.endswith(b"\n"):
        response += client.recv(65536)
    value = json.loads(response)
    if "error" in value:
        client.close()
        raise AssertionError(value["error"])
    return client


def wait_for_lost_notification(client: socket.socket) -> None:
    response = b""
    while True:
        response += client.recv(65536)
        while b"\n" in response:
            raw, response = response.split(b"\n", 1)
            value = json.loads(raw)
            if value.get("method") == "spatial.state_changed" and value["params"]["state"] == "LOST":
                return


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=Path, required=True)
    parser.add_argument("--socket", type=Path, required=True)
    parser.add_argument("--token", required=True)
    parser.add_argument("--output", type=Path, required=True)
    arguments = parser.parse_args()
    state = call(arguments.socket, arguments.token, "spatial.state", {})
    assert state["state"] in {"READY", "DETECTING"}
    subscription = subscribe(arguments.socket, arguments.token)
    detected = call(arguments.socket, arguments.token, "spatial.detect", {})
    assert detected["surface_candidates"] > 0
    selected = call(arguments.socket, arguments.token, "spatial.select", {"manual_confirmation": False})
    assert selected["state"] == "SELECTED"
    calibrated = call(arguments.socket, arguments.token, "spatial.calibrate", {"corners": selected["corners"]})
    assert calibrated["state"] == "TRACKING"
    call(arguments.socket, arguments.token, "spatial.source.start", {
        "source": "SIMULATION", "location": str(arguments.root / "assets/spatial-fixtures/no-surface.png"),
    })
    lost = call(arguments.socket, arguments.token, "spatial.detect", {})
    assert lost["state"] == "LOST"
    assert lost["last_error_code"] == 3305
    wait_for_lost_notification(subscription)
    subscription.close()
    arguments.output.parent.mkdir(parents=True, exist_ok=True)
    arguments.output.write_text(json.dumps({"status": "PASSED", "initial_state": state["state"], "final_state": lost["state"], "safe_pause_code": lost["last_error_code"]}) + "\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
