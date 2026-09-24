#!/usr/bin/env python3
"""Project-local JSON-RPC client for the P3 spatial target service."""

from __future__ import annotations

import argparse
import json
import socket
import sys
import uuid
from pathlib import Path


def request(socket_path: str, token: str, method: str, params: dict[str, object]) -> dict[str, object]:
    payload = {
        "jsonrpc": "2.0",
        "id": str(uuid.uuid4()),
        "trace_id": str(uuid.uuid4()),
        "method": method,
        "params": params,
        "security_context": {"capability_token": token},
    }
    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as connection:
        connection.settimeout(3.0)
        connection.connect(socket_path)
        connection.sendall(json.dumps(payload, separators=(",", ":")).encode("utf-8") + b"\n")
        response = b""
        while not response.endswith(b"\n"):
            chunk = connection.recv(65536)
            if not chunk:
                raise RuntimeError("spatial service closed the connection without a response")
            response += chunk
    decoded = json.loads(response.decode("utf-8"))
    if "error" in decoded:
        error = decoded["error"]
        raise RuntimeError(f"JSON-RPC error {error['code']}: {error['message']}")
    return decoded["result"]


def calibrate(socket_path: str, token: str) -> dict[str, object]:
    detected = request(socket_path, token, "spatial.detect", {})
    if int(detected.get("surface_candidates", 0)) < 1:
        raise RuntimeError("P3 did not detect a projection surface")
    selected = request(socket_path, token, "spatial.select", {"manual_confirmation": False})
    calibrated = request(socket_path, token, "spatial.calibrate", {"corners": selected["corners"]})
    target = calibrated.get("target", {})
    target_id = str(target.get("target_id", ""))
    if calibrated.get("state") != "TRACKING" or target.get("state") != "CALIBRATED":
        raise RuntimeError("P3 target did not reach TRACKING/CALIBRATED")
    try:
        uuid.UUID(target_id)
    except ValueError as error:
        raise RuntimeError("P3 calibrated target ID is not a UUID") from error
    return calibrated


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--socket", required=True)
    parser.add_argument("--token-file", required=True, type=Path)
    action = parser.add_mutually_exclusive_group(required=True)
    action.add_argument("--health", action="store_true")
    action.add_argument("--calibrate", action="store_true")
    action.add_argument("--state", action="store_true")
    args = parser.parse_args()
    token = args.token_file.read_text(encoding="utf-8").strip()
    try:
        if args.health:
            result = request(args.socket, token, "spatial.health", {})
        elif args.calibrate:
            result = calibrate(args.socket, token)
        else:
            result = request(args.socket, token, "spatial.state", {})
        print(json.dumps(result, sort_keys=True))
    except (OSError, RuntimeError, ValueError, KeyError, json.JSONDecodeError) as error:
        print(f"P3_TARGET_CLIENT_FAILED: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
