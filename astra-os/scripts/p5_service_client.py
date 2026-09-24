#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import socket
import uuid
from pathlib import Path

from generated_method_capabilities import capability_for_method


FIXTURE_WINDOW_ID = "d13d0ba9-8de1-4c38-97ae-09f9d56e9935"


def scoped_token(token: str, method: str) -> str:
    return hashlib.sha256(f"{token}:{capability_for_method(method)}".encode()).hexdigest()


def invoke(
    socket_path: str,
    method: str,
    params: dict[str, object],
    token: str,
    *,
    raw_token: bool = False,
    capability: str | None = None,
) -> dict[str, object]:
    request = {
        "jsonrpc": "2.0",
        "id": str(uuid.uuid4()),
        "trace_id": str(uuid.uuid4()),
        "method": method,
        "params": params,
        "security_context": {
            "capability_token": token
            if raw_token or method == "system.shutdown"
            else hashlib.sha256(f"{token}:{capability or capability_for_method(method)}".encode()).hexdigest()
        },
    }
    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as client:
        client.settimeout(2.0)
        client.connect(socket_path)
        client.sendall(json.dumps(request, separators=(",", ":")).encode() + b"\n")
        response = b""
        while not response.endswith(b"\n"):
            chunk = client.recv(65536)
            if not chunk:
                break
            response += chunk
    parsed = json.loads(response)
    if "error" in parsed:
        raise RuntimeError(f"{parsed['error']['code']}: {parsed['error']['message']}")
    return parsed["result"]


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--socket", required=True)
    parser.add_argument("--token-file", required=True, type=Path)
    parser.add_argument("--supervisor-token-file", type=Path)
    action = parser.add_mutually_exclusive_group(required=True)
    action.add_argument("--health", action="store_true")
    action.add_argument("--status", action="store_true")
    action.add_argument("--shutdown", action="store_true")
    action.add_argument("--create-public", action="store_true")
    action.add_argument("--create-private", action="store_true")
    action.add_argument("--target-lost", action="store_true")
    action.add_argument("--focus", metavar="COMPONENT_ID")
    action.add_argument("--open-fixture-window", metavar="COMPONENT_ID")
    action.add_argument("--move-fixture-window", action="store_true")
    action.add_argument("--resize-fixture-window", action="store_true")
    args = parser.parse_args()
    token_path = args.supervisor_token_file if args.shutdown else args.token_file
    if token_path is None:
        parser.error("--supervisor-token-file is required for shutdown")
    token = token_path.read_text(encoding="utf-8").strip()
    if args.health:
        result = invoke(args.socket, "system.health", {}, token)
    elif args.status:
        result = invoke(args.socket, "spatial_ui.status", {}, token)
    elif args.shutdown:
        result = invoke(args.socket, "system.shutdown", {}, token)
    elif args.target_lost:
        result = invoke(args.socket, "spatial_ui.target.lost", {}, token)
    elif args.focus:
        result = invoke(args.socket, "spatial_ui.focus", {"component_id": args.focus, "reason": "P5 graphics fixture focus"}, token)
    elif args.open_fixture_window:
        result = invoke(args.socket, "spatial_ui.window.open", {
            "window_id": FIXTURE_WINDOW_ID,
            "component_id": args.open_fixture_window,
            "bounds": {"x": 48, "y": 64, "width": 420, "height": 240},
            "display_target": "PROJECTION",
            "projection_target": "p4-fixture-target",
        }, token)
    elif args.move_fixture_window:
        result = invoke(args.socket, "spatial_ui.window.move", {"window_id": FIXTURE_WINDOW_ID, "x": 96, "y": 80}, token)
    elif args.resize_fixture_window:
        result = invoke(socket_path=args.socket, method="spatial_ui.window.resize",
                        params={"window_id": FIXTURE_WINDOW_ID, "width": 460, "height": 260}, token=token)
    else:
        privacy = "PUBLIC" if args.create_public else "PRIVATE_SCREEN_ONLY"
        result = invoke(args.socket, "spatial_ui.task.create", {
            "schema_version": "1.0",
            "task_id": "7abf7b1e-dc4f-4b7c-96cc-c3dd628e1ca9" if args.create_public else str(uuid.uuid4()),
            "title": f"P5 {privacy} fixture task",
            "summary": "Deterministic P2-style fixture",
            "intent_type": "open_task_surface",
            "confidence": 1.0,
            "execution_strategy": "fixture_adapter",
            "privacy_level": privacy,
            "bounds": {"x": 32, "y": 96, "width": 360, "height": 180},
            "display_target": "BOTH",
            "accessibility_label": f"P5 {privacy} fixture task",
        }, token)
    print(json.dumps(result, ensure_ascii=False, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
