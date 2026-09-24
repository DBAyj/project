#!/usr/bin/env python3
from __future__ import annotations

import json
import hashlib
from pathlib import Path
import secrets
import socket
import subprocess
import tempfile
import time
import uuid

from generated_method_capabilities import capability_for_method


ROOT = Path(__file__).resolve().parents[1]
P4_SERVICE = ROOT / "runtime/tmp/p5-build/services/astra-projection-service/astra-projection-service"
P5_SERVICE = ROOT / "runtime/tmp/p5-build/services/astra-spatial-ui-service/astra-spatial-ui-service"
MARKER = "P4_RELEASE_BASELINE_FINAL"
PUBLIC_FIXTURE_SUBJECT_ID = "7abf7b1e-dc4f-4b7c-96cc-c3dd628e1ca9"


def scoped_token(token: str, method: str) -> str:
    return hashlib.sha256(f"{token}:{capability_for_method(method)}".encode()).hexdigest()


def invoke(socket_path: str, method: str, params: dict[str, object], token: str, capability: str | None = None) -> dict[str, object]:
    payload = {
        "jsonrpc": "2.0",
        "id": str(uuid.uuid4()),
        "trace_id": str(uuid.uuid4()),
        "method": method,
        "params": params,
        "security_context": {"capability_token": token if method == "system.shutdown" else hashlib.sha256(
            f"{token}:{capability or capability_for_method(method)}".encode()
        ).hexdigest()},
    }
    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as client:
        client.settimeout(2)
        client.connect(socket_path)
        client.sendall(json.dumps(payload, separators=(",", ":")).encode() + b"\n")
        data = b""
        while not data.endswith(b"\n"):
            chunk = client.recv(65536)
            if not chunk:
                raise RuntimeError("service closed without a response")
            data += chunk
    return json.loads(data)


def wait_for_health(socket_path: str, token: str, capability: str | None = None) -> None:
    for _ in range(100):
        try:
            response = invoke(socket_path, "system.health", {}, token, capability)
            if response.get("result", {}).get("status") == "HEALTHY":
                return
        except (FileNotFoundError, ConnectionRefusedError, TimeoutError, OSError):
            time.sleep(0.02)
    raise RuntimeError(f"service did not become healthy: {socket_path}")


def write_token(path: Path, token: str) -> None:
    path.write_text(token + "\n", encoding="utf-8")
    path.chmod(0o600)


def component(privacy: str) -> dict[str, object]:
    return {
        "schema_version": "1.0",
        "task_id": PUBLIC_FIXTURE_SUBJECT_ID if privacy == "PUBLIC" else str(uuid.uuid4()),
        "title": f"{privacy} security fixture",
        "summary": "Deterministic policy fixture",
        "intent_type": "open_task_surface",
        "confidence": 1.0,
        "execution_strategy": "fixture_adapter",
        "privacy_level": privacy,
        "accessibility_label": f"{privacy} security fixture",
        "bounds": {"x": 20, "y": 20, "width": 320, "height": 180},
        "display_target": "BOTH",
    }


def main() -> int:
    if not P4_SERVICE.is_file() or not P5_SERVICE.is_file():
        print("FAIL: build P4 and P5 before privacy verification")
        return 1
    with tempfile.TemporaryDirectory() as directory:
        root = Path(directory)
        p4_socket = f"/tmp/astra-p5-security-p4-{uuid.uuid4().hex[:8]}.sock"
        p5_socket = f"/tmp/astra-p5-security-ui-{uuid.uuid4().hex[:8]}.sock"
        p4_token = secrets.token_urlsafe(48)
        client_token = secrets.token_urlsafe(48)
        supervisor_token = secrets.token_urlsafe(48)
        p4_token_path = root / "p4.token"
        client_token_path = root / "client.token"
        supervisor_token_path = root / "supervisor.token"
        write_token(p4_token_path, p4_token)
        write_token(client_token_path, client_token)
        write_token(supervisor_token_path, supervisor_token)
        audit = root / "audit.jsonl"
        p4 = subprocess.Popen(
            [str(P4_SERVICE), "--socket", p4_socket, "--capability-token-file", str(p4_token_path)],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )
        p5: subprocess.Popen[bytes] | None = None
        try:
            wait_for_health(p4_socket, p4_token, capability="system.health.read")
            p5 = subprocess.Popen(
                [
                    str(P5_SERVICE), "--project-root", str(ROOT), "--socket", p5_socket, "--state", str(root / "state.json"),
                    "--audit", str(audit), "--client-token-file", str(client_token_path),
                    "--supervisor-token-file", str(supervisor_token_path), "--projection-socket", p4_socket,
                    "--projection-token-file", str(p4_token_path),
                ],
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
            )
            wait_for_health(p5_socket, client_token)
            decisions = {}
            for privacy in ("PUBLIC", "PRIVATE_SCREEN_ONLY", "NO_PROJECTION"):
                response = invoke(p5_socket, "spatial_ui.task.create", component(privacy), client_token)
                decisions[privacy] = response["result"]["projection_layer_generated"]
            if decisions != {"PUBLIC": True, "PRIVATE_SCREEN_ONLY": False, "NO_PROJECTION": False}:
                raise RuntimeError(f"privacy decision mismatch: {decisions}")
            projected = invoke(p4_socket, "projection.output.frame", {"output_id": "p4-window-projection"}, p4_token,
                               capability="projection.output.read")
            labels = projected.get("result", {}).get("public_labels", [])
            if "PUBLIC security fixture" not in labels:
                raise RuntimeError(f"readable public content did not reach P4: {labels}")
            if any("PRIVATE_SCREEN_ONLY" in label or "NO_PROJECTION" in label for label in labels):
                raise RuntimeError(f"private content leaked into P4 labels: {labels}")
            denied = invoke(p5_socket, "spatial_ui.status", {}, "tampered-token")
            if denied.get("error", {}).get("code") != 5002:
                raise RuntimeError("tampered capability token was not rejected")
            invoke(p5_socket, "spatial_ui.target.lost", {}, client_token)
            if not invoke(p5_socket, "spatial_ui.status", {}, client_token)["result"]["projection_safe"]:
                raise RuntimeError("target loss did not enter projection-safe state")
            frame = invoke(p4_socket, "projection.output.frame", {"output_id": "p4-window-projection"}, p4_token,
                           capability="projection.output.read")
            if frame.get("error", {}).get("code") != 4018:
                raise RuntimeError("P4 retained a stale frame after target loss")
            invoke(p5_socket, "system.shutdown", {}, supervisor_token)
            if p5.wait(timeout=3) != 0:
                raise RuntimeError("P5 shutdown failed")
            if Path(p5_socket).exists():
                raise RuntimeError("P5 socket remained after shutdown")
            entries = [json.loads(line) for line in audit.read_text(encoding="utf-8").splitlines() if line]
            if not entries or any(entry.get("p4_release_status") != MARKER for entry in entries):
                raise RuntimeError("audit marker is missing")
            required = {"event_id", "trace_id", "request_id", "session_id", "payload"}
            if any(not required.issubset(entry) for entry in entries):
                raise RuntimeError("audit identifiers are incomplete")
        except Exception as error:
            print(f"FAIL: {error}")
            return 1
        finally:
            if p5 is not None and p5.poll() is None:
                p5.terminate()
                p5.wait(timeout=3)
            if p4.poll() is None:
                p4.terminate()
                p4.wait(timeout=3)
            Path(p4_socket).unlink(missing_ok=True)
            Path(p5_socket).unlink(missing_ok=True)
    print("PASS: real P4/P5 privacy isolation, capability denial, target-loss clear, audit parsing, and socket cleanup verified")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
