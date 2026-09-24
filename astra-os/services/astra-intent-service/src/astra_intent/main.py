from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import signal
import threading

from astra_intent.application.intent_service import IntentService
from astra_intent.transport.http_app import DevelopmentHttpServer
from astra_intent.transport.jsonrpc_server import JsonRpcDispatcher, UnixJsonRpcServer


def repository_root() -> Path:
    return Path(__file__).resolve().parents[4]


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="AstraOS P2 intent service")
    parser.add_argument("--root", type=Path, default=repository_root())
    parser.add_argument("--socket", type=Path)
    parser.add_argument("--http-host")
    parser.add_argument("--http-port", type=int)
    parser.add_argument("--disable-http", action="store_true")
    return parser.parse_args()


def main() -> int:
    arguments = parse_arguments()
    root = arguments.root.resolve()
    service = IntentService.from_repository(root)
    transport = service.transport_configuration()
    token = os.environ.get("ASTRA_INTENT_CAPABILITY_TOKEN", "")
    if not token:
        raise RuntimeError("ASTRA_INTENT_CAPABILITY_TOKEN is required")
    configured_socket = Path(str(transport["socket_path"]))
    socket_path = arguments.socket or (root / configured_socket)
    dispatcher = JsonRpcDispatcher(service, capability_token=token)
    unix_server = UnixJsonRpcServer(socket_path, dispatcher)
    http_server: DevelopmentHttpServer | None = None
    http_host: str | None = None
    if transport["http_enabled"] and not arguments.disable_http:
        host = arguments.http_host or str(transport["http_host"])
        if not host:
            raise RuntimeError("Development HTTP requires --http-host with an explicit interface address")
        http_host = host
        port = arguments.http_port or int(transport["http_port"])
        http_server = DevelopmentHttpServer(host, port, service, capability_token=token)

    stopped = threading.Event()

    def request_stop(_signum: int, _frame: object) -> None:
        stopped.set()

    signal.signal(signal.SIGINT, request_stop)
    signal.signal(signal.SIGTERM, request_stop)
    unix_server.start()
    if http_server is not None:
        http_server.start()
    print(
        json.dumps(
            {
                "event": "intent_service_started",
                "socket_path": str(socket_path),
                "http_host": http_host,
                "http_port": http_server.port if http_server is not None else None,
            },
            ensure_ascii=False,
        ),
        flush=True,
    )
    try:
        stopped.wait()
    finally:
        if http_server is not None:
            http_server.stop()
        unix_server.stop()
    print(json.dumps({"event": "intent_service_stopped"}), flush=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
