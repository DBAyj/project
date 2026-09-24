from __future__ import annotations

import asyncio
import base64
from hashlib import sha1
from http import HTTPStatus
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import json
import threading
from typing import Any

from astra_intent.application.intent_service import IntentService
from astra_intent.domain.errors import IntentServiceError


class DevelopmentHttpServer:
    def __init__(self, host: str, port: int, service: IntentService, *, capability_token: str) -> None:
        if not host:
            raise ValueError("Development HTTP requires an explicit bind address")
        if not capability_token:
            raise ValueError("Development HTTP requires a capability token")
        handler = self._handler(service, capability_token)
        self._server = ThreadingHTTPServer((host, port), handler)
        self._thread: threading.Thread | None = None

    @property
    def port(self) -> int:
        return int(self._server.server_address[1])

    def start(self) -> None:
        self._thread = threading.Thread(target=self._server.serve_forever, name="astra-intent-http", daemon=True)
        self._thread.start()

    def stop(self) -> None:
        self._server.shutdown()
        self._server.server_close()
        if self._thread is not None:
            self._thread.join(timeout=2)
        self._thread = None

    @staticmethod
    def _handler(service: IntentService, token: str) -> type[BaseHTTPRequestHandler]:
        class Handler(BaseHTTPRequestHandler):
            def do_GET(self) -> None:
                if not self._authorized():
                    return
                if self.path == "/events":
                    self._write_websocket_event()
                    return
                routes: dict[str, object] = {
                    "/health": service.health(),
                    "/ready": service.ready(),
                    "/v2/intents/supported": {"intents": service.supported_intents()},
                    "/metrics": service.metrics(),
                }
                if self.path not in routes:
                    self._write(HTTPStatus.NOT_FOUND, {"error": "not_found"})
                    return
                self._write(HTTPStatus.OK, routes[self.path])

            def do_POST(self) -> None:
                if not self._authorized():
                    return
                try:
                    length = int(self.headers.get("Content-Length", "0"))
                    body = json.loads(self.rfile.read(length))
                    if not isinstance(body, dict):
                        raise ValueError("JSON body must be an object")
                    if self.path == "/v2/intents/parse":
                        result = asyncio.run(service.parse(body))
                    elif self.path == "/v2/intents/confirm":
                        result = service.confirm(body["confirmation_id"])
                    elif self.path == "/v2/intents/reject":
                        result = service.reject(body["confirmation_id"])
                    else:
                        self._write(HTTPStatus.NOT_FOUND, {"error": "not_found"})
                        return
                    self._write(HTTPStatus.OK, result)
                except IntentServiceError as error:
                    self._write(HTTPStatus.CONFLICT, {"error": {"code": error.code, "message": service.error_message(error.code)}})
                except (json.JSONDecodeError, KeyError, TypeError, ValueError):
                    self._write(HTTPStatus.BAD_REQUEST, {"error": {"code": 2009, "message": service.error_message(2009)}})

            def _authorized(self) -> bool:
                if self.headers.get("X-Astra-Capability") == token:
                    return True
                self._write(HTTPStatus.FORBIDDEN, {"error": {"code": 5001, "message": service.error_message(5001)}})
                return False

            def _write(self, status: HTTPStatus, value: object) -> None:
                payload = json.dumps(value, ensure_ascii=False, separators=(",", ":")).encode("utf-8")
                self.send_response(status.value)
                self.send_header("Content-Type", "application/json; charset=utf-8")
                self.send_header("Content-Length", str(len(payload)))
                self.end_headers()
                self.wfile.write(payload)

            def _write_websocket_event(self) -> None:
                if self.headers.get("Upgrade", "").casefold() != "websocket":
                    self._write(HTTPStatus.UPGRADE_REQUIRED, {"error": "websocket_upgrade_required"})
                    return
                key = self.headers.get("Sec-WebSocket-Key", "")
                if not key:
                    self._write(HTTPStatus.BAD_REQUEST, {"error": "missing_websocket_key"})
                    return
                accept = base64.b64encode(sha1((key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11").encode()).digest()).decode()
                self.send_response(HTTPStatus.SWITCHING_PROTOCOLS.value)
                self.send_header("Upgrade", "websocket")
                self.send_header("Connection", "Upgrade")
                self.send_header("Sec-WebSocket-Accept", accept)
                self.end_headers()
                payload = json.dumps(
                    {"event": "intent_service_connected", "health": service.health(), "metrics": service.metrics()},
                    ensure_ascii=False,
                    separators=(",", ":"),
                ).encode("utf-8")
                if len(payload) < 126:
                    frame = bytes((0x81, len(payload))) + payload
                else:
                    frame = bytes((0x81, 126)) + len(payload).to_bytes(2, "big") + payload
                self.wfile.write(frame)
                self.wfile.flush()

            def log_message(self, format: str, *args: Any) -> None:
                del format, args

        return Handler
