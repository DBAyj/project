from __future__ import annotations

import asyncio
import json
import os
from pathlib import Path
import socket
import threading
from typing import Any

from astra_intent.application.intent_service import IntentService
from astra_intent.domain.errors import IntentServiceError


class JsonRpcDispatcher:
    _allowed_fields = {"jsonrpc", "id", "trace_id", "method", "params", "security_context"}
    _required_fields = {"jsonrpc", "id", "method", "params", "security_context"}

    def __init__(self, service: IntentService, *, capability_token: str) -> None:
        if not capability_token:
            raise ValueError("A non-empty capability token is required")
        self._service = service
        self._capability_token = capability_token

    def dispatch(self, request: dict[str, Any]) -> dict[str, Any]:
        request_id = request.get("id")
        if not self._valid_envelope(request):
            return self._error(request_id, -32600, "Invalid Request")
        security = request["security_context"]
        if security.get("capability_token") != self._capability_token:
            return self._error(request_id, 5001, self._service.error_message(5001))
        try:
            result = self._invoke(request["method"], request["params"])
            return {"jsonrpc": "2.0", "id": request_id, "result": result}
        except IntentServiceError as error:
            return self._error(request_id, error.code, self._service.error_message(error.code))
        except (KeyError, TypeError, ValueError):
            return self._error(request_id, -32602, "Invalid params")

    def _invoke(self, method: str, params: dict[str, Any]) -> object:
        if method == "intent.parse":
            return asyncio.run(self._service.parse(params))
        if method == "intent.confirm":
            return self._service.confirm(params["confirmation_id"])
        if method == "intent.reject":
            return self._service.reject(params["confirmation_id"])
        if method == "intent.supported":
            return {"intents": self._service.supported_intents()}
        if method == "system.health":
            return self._service.health()
        if method == "system.ready":
            return self._service.ready()
        if method == "intent.metrics":
            return self._service.metrics()
        raise IntentServiceError(-32601, "Method not found")

    def _valid_envelope(self, request: dict[str, Any]) -> bool:
        if set(request) - self._allowed_fields or not self._required_fields.issubset(request):
            return False
        return (
            request["jsonrpc"] == "2.0"
            and isinstance(request["id"], str)
            and bool(request["id"])
            and isinstance(request["method"], str)
            and isinstance(request["params"], dict)
            and isinstance(request["security_context"], dict)
            and set(request["security_context"]).issubset({"capability_token", "delegation_id"})
        )

    @staticmethod
    def _error(request_id: object, code: int, message: str) -> dict[str, Any]:
        return {"jsonrpc": "2.0", "id": request_id, "error": {"code": code, "message": message, "data": {}}}


class UnixJsonRpcServer:
    def __init__(self, path: Path, dispatcher: JsonRpcDispatcher) -> None:
        self.path = path
        self._dispatcher = dispatcher
        self._socket: socket.socket | None = None
        self._thread: threading.Thread | None = None
        self._stop = threading.Event()
        self._inode: int | None = None

    def start(self) -> None:
        self.path.parent.mkdir(parents=True, exist_ok=True)
        self._remove_stale_socket()
        server = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        try:
            server.bind(str(self.path))
            os.chmod(self.path, 0o600)
            server.listen(16)
            server.settimeout(0.2)
        except Exception:
            server.close()
            raise
        self._socket = server
        self._inode = self.path.stat().st_ino
        self._stop.clear()
        self._thread = threading.Thread(target=self._serve, name="astra-intent-unix", daemon=True)
        self._thread.start()

    def stop(self) -> None:
        self._stop.set()
        if self._socket is not None:
            self._socket.close()
        if self._thread is not None:
            self._thread.join(timeout=2)
        self._socket = None
        self._thread = None
        try:
            if self._inode is not None and self.path.stat().st_ino == self._inode:
                self.path.unlink()
        except FileNotFoundError:
            pass
        self._inode = None

    def _serve(self) -> None:
        assert self._socket is not None
        while not self._stop.is_set():
            try:
                connection, _ = self._socket.accept()
            except (TimeoutError, OSError):
                continue
            with connection:
                reader = connection.makefile("rb")
                for line in reader:
                    try:
                        value = json.loads(line)
                        response = self._dispatcher.dispatch(value) if isinstance(value, dict) else JsonRpcDispatcher._error(None, -32600, "Invalid Request")
                    except (json.JSONDecodeError, UnicodeDecodeError):
                        response = JsonRpcDispatcher._error(None, -32700, "Parse error")
                    connection.sendall((json.dumps(response, ensure_ascii=False, separators=(",", ":")) + "\n").encode("utf-8"))

    def _remove_stale_socket(self) -> None:
        if not self.path.exists():
            return
        probe = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        try:
            probe.settimeout(0.2)
            probe.connect(str(self.path))
        except OSError:
            self.path.unlink()
        else:
            raise RuntimeError(f"Intent service socket is already active: {self.path}")
        finally:
            probe.close()
