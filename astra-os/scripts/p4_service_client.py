#!/usr/bin/env python3
"""Small project-local JSON-RPC client for the P4 Unix-socket service."""

from __future__ import annotations

import argparse
import hashlib
import json
import socket
import statistics
import sys
import time
import uuid
from pathlib import Path

from generated_method_capabilities import capability_for_method


MARKER = "P3_REAL_SPATIAL_INTEGRATION_NOT_VERIFIED"
DEFAULT_TOKEN = "astra-p4-fixture-capability"


def request(socket_path: str, method: str, params: dict[str, object], token: str | None = None) -> dict[str, object]:
    token = token or DEFAULT_TOKEN
    payload = {
        "jsonrpc": "2.0",
        "id": str(uuid.uuid4()),
        "trace_id": str(uuid.uuid4()),
        "method": method,
        "params": params,
        "security_context": {
            "capability_token": hashlib.sha256(f"{token}:{capability_for_method(method)}".encode()).hexdigest()
        },
    }
    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as connection:
        connection.settimeout(3.0)
        connection.connect(socket_path)
        connection.sendall(json.dumps(payload, separators=(",", ":")).encode("utf-8") + b"\n")
        response = b""
        while not response.endswith(b"\n"):
            chunk = connection.recv(65536)
            if not chunk:
                raise RuntimeError("projection service closed the connection without a response")
            response += chunk
    decoded = json.loads(response.decode("utf-8"))
    if "error" in decoded:
        error = decoded["error"]
        raise RuntimeError(f"JSON-RPC error {error['code']}: {error['message']}")
    return decoded["result"]


def start(socket_path: str) -> dict[str, object]:
    session_id = str(uuid.uuid4())
    request(socket_path, "projection.session.command", {"session_id": session_id, "command": "INITIALIZE"})
    request(socket_path, "projection.session.command", {"session_id": session_id, "command": "READY"})
    return request(
        socket_path,
        "projection.render",
        {
            "request_id": str(uuid.uuid4()),
            "trace_id": str(uuid.uuid4()),
            "session_id": session_id,
            "output_id": "p4-window-projection",
            "fixture_id": "front-rectangle",
            "layer_ids": [str(uuid.uuid4())],
            "privacy_level": "PUBLIC",
            "p3_integration_status": MARKER,
        },
    )


def stop(socket_path: str) -> dict[str, object]:
    return request(socket_path, "projection.session.command", {"session_id": str(uuid.uuid4()), "command": "STOP"})


def benchmark(socket_path: str, iterations: int) -> dict[str, float | int]:
    durations: list[float] = []
    first_frame_ms: float | None = None
    for _ in range(iterations):
        began = time.perf_counter()
        result = request(
            socket_path,
            "projection.render",
            {
                "request_id": str(uuid.uuid4()),
                "trace_id": str(uuid.uuid4()),
                "session_id": str(uuid.uuid4()),
                "output_id": "p4-window-projection",
                "fixture_id": "front-rectangle",
                "layer_ids": [str(uuid.uuid4())],
                "privacy_level": "PUBLIC",
                "p3_integration_status": MARKER,
            },
        )
        elapsed_ms = (time.perf_counter() - began) * 1000.0
        durations.append(elapsed_ms)
        if first_frame_ms is None and result["frame_id"]:
            first_frame_ms = elapsed_ms
    began = time.perf_counter()
    stop(socket_path)
    clear_latency_ms = (time.perf_counter() - began) * 1000.0
    ordered = sorted(durations)
    percentile_index = max(0, min(len(ordered) - 1, int(len(ordered) * 0.95 + 0.5) - 1))
    return {
        "frames": iterations,
        "p95_ms": ordered[percentile_index],
        "fps": 1000.0 / statistics.fmean(durations),
        "first_frame_ms": first_frame_ms or 0.0,
        "safe_clear_ms": clear_latency_ms,
    }


def cycles(socket_path: str, count: int) -> None:
    try:
        stop(socket_path)
    except RuntimeError:
        pass
    for _ in range(count):
        start(socket_path)
        stop(socket_path)


def main() -> int:
    global DEFAULT_TOKEN
    parser = argparse.ArgumentParser()
    parser.add_argument("--socket", required=True)
    parser.add_argument("--token-file", type=Path)
    parser.add_argument("--health", action="store_true")
    parser.add_argument("--demo", action="store_true")
    parser.add_argument("--stop", action="store_true")
    parser.add_argument("--frame", action="store_true")
    parser.add_argument("--output-id", default="p4-window-projection")
    parser.add_argument("--benchmark", type=int)
    parser.add_argument("--cycles", type=int)
    parser.add_argument("--report", type=Path)
    args = parser.parse_args()
    token = args.token_file.read_text(encoding="utf-8").strip() if args.token_file else DEFAULT_TOKEN
    DEFAULT_TOKEN = token
    try:
        if args.health:
            print(json.dumps(request(args.socket, "system.health", {}), sort_keys=True))
        elif args.demo:
            print(json.dumps(start(args.socket), sort_keys=True))
        elif args.stop:
            print(json.dumps(stop(args.socket), sort_keys=True))
        elif args.frame:
            print(json.dumps(
                request(args.socket, "projection.output.frame", {"output_id": args.output_id}),
                sort_keys=True,
            ))
        elif args.benchmark is not None:
            metrics = benchmark(args.socket, args.benchmark)
            if args.report is not None:
                args.report.parent.mkdir(parents=True, exist_ok=True)
                args.report.write_text(json.dumps(metrics, indent=2, sort_keys=True) + "\n", encoding="utf-8")
            print(json.dumps(metrics, sort_keys=True))
        elif args.cycles is not None:
            cycles(args.socket, args.cycles)
            print(f"P4_STABILITY_CYCLES_PASSED={args.cycles}")
        else:
            parser.error("select one action")
    except (OSError, RuntimeError, ValueError, KeyError) as error:
        print(f"P4_SERVICE_CLIENT_FAILED: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
