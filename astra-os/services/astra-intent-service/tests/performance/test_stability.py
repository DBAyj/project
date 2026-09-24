from __future__ import annotations

import asyncio
import json
from pathlib import Path
import resource
import sys
import tempfile
from time import perf_counter
import unittest
from uuid import uuid4

from jsonschema import Draft202012Validator, FormatChecker

from astra_intent.application.intent_service import IntentService
from astra_intent.transport.jsonrpc_server import JsonRpcDispatcher, UnixJsonRpcServer


def rss_megabytes() -> float:
    value = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss
    return value / (1024 * 1024) if sys.platform == "darwin" else value / 1024


class IntentStabilityTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.root = Path(__file__).resolve().parents[4]

    @staticmethod
    def request(text: str) -> dict[str, object]:
        return {
            "schema_version": "2.0",
            "request_id": str(uuid4()),
            "session_id": str(uuid4()),
            "user_id": "local-user",
            "locale": "zh-CN",
            "raw_text": text,
            "current_context": {
                "projection_state": "IDLE",
                "current_space": "development_workspace",
                "current_privacy_level": "PUBLIC",
                "current_model_id": "demo-device",
            },
            "client": {"name": "astra-shell", "version": "0.2.0-alpha.1"},
            "requested_at": "2026-07-14T13:40:00Z",
        }

    def test_ten_thousand_requests_leave_clean_runtime_state(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            runtime = Path(directory)
            audit_path = runtime / "stability.jsonl"
            service = IntentService.from_repository(self.root, audit_path=audit_path)
            commands = ("把设备模型投到桌面上", "查看系统状态", "旋转模型", "放大模型", "帮助")
            rss_before = rss_megabytes()
            started = perf_counter()

            async def exercise() -> int:
                failures = 0
                for index in range(10000):
                    result = await service.parse(self.request(commands[index % len(commands)]))
                    failures += int(result["execution_policy"] == "REJECT")
                return failures

            failures = asyncio.run(exercise())
            elapsed = perf_counter() - started
            rss_after = rss_megabytes()
            lines = [json.loads(line) for line in audit_path.read_text(encoding="utf-8").splitlines() if line]
            schema = json.loads((self.root / "protocols/intent/intent-event-v1.schema.json").read_text(encoding="utf-8"))
            validator = Draft202012Validator(schema, format_checker=FormatChecker())
            schema_errors = sum(bool(list(validator.iter_errors(line))) for line in lines)

            socket_path = runtime / "astra-intent.sock"
            server = UnixJsonRpcServer(socket_path, JsonRpcDispatcher(service, capability_token="stability-token"))
            server.start()
            self.assertTrue(socket_path.exists())
            server.stop()
            self.assertFalse(socket_path.exists())

            report = {
                "requests": 10000,
                "failures": failures,
                "elapsed_seconds": round(elapsed, 3),
                "throughput_rps": round(10000 / elapsed, 3),
                "audit_records": len(lines),
                "audit_schema_errors": schema_errors,
                "memory_growth_mb": round(max(0.0, rss_after - rss_before), 3),
                "socket_cleaned": not socket_path.exists(),
            }
            output = self.root / "runtime/tmp/p2-stability.json"
            output.parent.mkdir(parents=True, exist_ok=True)
            output.write_text(json.dumps(report, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")

            self.assertEqual(failures, 0)
            self.assertEqual(schema_errors, 0)
            self.assertLessEqual(report["memory_growth_mb"], 64.0)


if __name__ == "__main__":
    unittest.main()
