from __future__ import annotations

import json
from pathlib import Path
import socket
import tempfile
import unittest
from urllib.error import HTTPError
from urllib.request import Request, urlopen
from uuid import uuid4

from astra_intent.application.intent_service import IntentService
from astra_intent.transport.http_app import DevelopmentHttpServer
from astra_intent.transport.jsonrpc_server import JsonRpcDispatcher, UnixJsonRpcServer


class TransportIntegrationTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.root = Path(__file__).resolve().parents[4]
        cls.token = "test-capability-token"

    def setUp(self) -> None:
        self.service = IntentService.from_repository(self.root)
        self.dispatcher = JsonRpcDispatcher(self.service, capability_token=self.token)

    def _intent_request(self) -> dict[str, object]:
        return {
            "schema_version": "2.0",
            "request_id": str(uuid4()),
            "session_id": str(uuid4()),
            "user_id": "local-user",
            "locale": "zh-CN",
            "raw_text": "把设备模型投到桌面上",
            "current_context": {
                "projection_state": "IDLE",
                "current_space": "development_workspace",
                "current_privacy_level": "PUBLIC",
                "current_model_id": "demo-device",
            },
            "client": {"name": "astra-shell", "version": "0.2.0-alpha.1"},
            "requested_at": "2026-07-14T13:40:00Z",
        }

    def _rpc(self, method: str, params: dict[str, object], *, token: str | None = None) -> dict[str, object]:
        return {
            "jsonrpc": "2.0",
            "id": str(uuid4()),
            "method": method,
            "params": params,
            "security_context": {"capability_token": self.token if token is None else token},
        }

    def test_unix_socket_parses_jsonrpc_and_cleans_up(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "astra-intent.sock"
            server = UnixJsonRpcServer(path, self.dispatcher)
            server.start()
            try:
                with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as client:
                    client.connect(str(path))
                    client.sendall((json.dumps(self._rpc("intent.parse", self._intent_request())) + "\n").encode())
                    response = json.loads(client.makefile("rb").readline())
                self.assertEqual(response["result"]["intent"], "project_3d_model")
                self.assertEqual(path.stat().st_mode & 0o777, 0o600)
            finally:
                server.stop()
            self.assertFalse(path.exists())

    def test_unix_socket_idle_client_does_not_block_other_clients(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "astra-intent.sock"
            server = UnixJsonRpcServer(path, self.dispatcher)
            server.CONNECTION_IDLE_TIMEOUT_SECONDS = 0.2
            server.start()
            try:
                with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as idle:
                    idle.connect(str(path))
                    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as client:
                        client.settimeout(5.0)
                        client.connect(str(path))
                        client.sendall((json.dumps(self._rpc("intent.parse", self._intent_request())) + "\n").encode())
                        response = json.loads(client.makefile("rb").readline())
                self.assertEqual(response["result"]["intent"], "project_3d_model")
            finally:
                server.stop()

    def test_unix_socket_rejects_malformed_json(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "astra-intent.sock"
            server = UnixJsonRpcServer(path, self.dispatcher)
            server.start()
            try:
                with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as client:
                    client.connect(str(path))
                    client.sendall(b"{not-json}\n")
                    response = json.loads(client.makefile("rb").readline())
                self.assertEqual(response["error"]["code"], -32700)
            finally:
                server.stop()

    def test_jsonrpc_rejects_missing_capability_and_unknown_method(self) -> None:
        denied = self.dispatcher.dispatch(self._rpc("intent.supported", {}, token="wrong"))
        unknown = self.dispatcher.dispatch(self._rpc("intent.missing", {}))

        self.assertEqual(denied["error"]["code"], 5001)
        self.assertEqual(unknown["error"]["code"], -32601)

    def test_development_http_health_and_parse_require_capability(self) -> None:
        server = DevelopmentHttpServer("127.0.0.1", 0, self.service, capability_token=self.token)
        server.start()
        try:
            base = f"http://127.0.0.1:{server.port}"
            health_request = Request(f"{base}/health", headers={"X-Astra-Capability": self.token})
            health = json.loads(urlopen(health_request, timeout=2).read())
            self.assertEqual(health["status"], "healthy")

            parse_request = Request(
                f"{base}/v2/intents/parse",
                data=json.dumps(self._intent_request()).encode(),
                headers={"Content-Type": "application/json", "X-Astra-Capability": self.token},
                method="POST",
            )
            result = json.loads(urlopen(parse_request, timeout=2).read())
            self.assertEqual(result["intent"], "project_3d_model")

            with self.assertRaises(HTTPError) as denied:
                urlopen(Request(f"{base}/health"), timeout=2)
            self.assertEqual(denied.exception.code, 403)
        finally:
            server.stop()

    def test_development_websocket_performs_real_upgrade_and_sends_text_frame(self) -> None:
        server = DevelopmentHttpServer("127.0.0.1", 0, self.service, capability_token=self.token)
        server.start()
        try:
            with socket.create_connection(("127.0.0.1", server.port), timeout=2) as client:
                request = (
                    "GET /events HTTP/1.1\r\n"
                    f"Host: 127.0.0.1:{server.port}\r\n"
                    "Upgrade: websocket\r\n"
                    "Connection: Upgrade\r\n"
                    "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
                    "Sec-WebSocket-Version: 13\r\n"
                    f"X-Astra-Capability: {self.token}\r\n\r\n"
                )
                client.sendall(request.encode("ascii"))
                response = b""
                while b"\r\n\r\n" not in response:
                    response += client.recv(4096)
                headers, frame = response.split(b"\r\n\r\n", 1)
                while len(frame) < 2:
                    frame += client.recv(4096)
                length = frame[1] & 0x7F
                header_length = 2
                if length == 126:
                    while len(frame) < 4:
                        frame += client.recv(4096)
                    length = int.from_bytes(frame[2:4], "big")
                    header_length = 4
                while len(frame) < header_length + length:
                    frame += client.recv(4096)
            self.assertIn(b"101 Switching Protocols", headers)
            self.assertIn(b"Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=", headers)
            self.assertEqual(frame[0], 0x81)
            self.assertIn(b"intent_service_connected", frame)
        finally:
            server.stop()


if __name__ == "__main__":
    unittest.main()
