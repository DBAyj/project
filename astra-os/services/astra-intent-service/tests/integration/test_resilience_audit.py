from __future__ import annotations

import json
from pathlib import Path
import tempfile
import unittest
from uuid import uuid4

from jsonschema import Draft202012Validator, FormatChecker

from astra_intent.adapters.deterministic_model import UnavailableIntentModel
from astra_intent.application.intent_service import IntentService
from astra_intent.engines.rule_engine import UnavailableRuleEngine


class ResilienceAuditIntegrationTests(unittest.IsolatedAsyncioTestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.root = Path(__file__).resolve().parents[4]
        cls.audit_schema = json.loads((cls.root / "protocols/intent/intent-event-v1.schema.json").read_text(encoding="utf-8"))

    def _request(self, text: str) -> dict[str, object]:
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

    async def test_cache_metrics_and_redacted_audit_are_observable(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            audit_path = Path(directory) / "intent.jsonl"
            service = IntentService.from_repository(self.root, audit_path=audit_path)
            first_request = self._request("把设备模型投到桌面上")
            first = await service.parse(first_request)
            second = await service.parse(self._request("把设备模型投到桌面上"))
            sensitive = await service.parse(self._request("联系13812341234后显示设备模型"))
            sensitive_second = await service.parse(self._request("联系13812341234后显示设备模型"))

            self.assertFalse(first["processing"]["cache_hit"])
            self.assertTrue(second["processing"]["cache_hit"])
            self.assertNotEqual(first["trace_id"], second["trace_id"])
            self.assertFalse(sensitive["processing"]["cache_hit"])
            self.assertFalse(sensitive_second["processing"]["cache_hit"])
            self.assertEqual(service.metrics()["requests_total"], 4)

            lines = [json.loads(line) for line in audit_path.read_text(encoding="utf-8").splitlines() if line]
            self.assertGreater(len(lines), 4)
            validator = Draft202012Validator(self.audit_schema, format_checker=FormatChecker())
            self.assertTrue(all(not list(validator.iter_errors(line)) for line in lines))
            self.assertNotIn("13812341234", audit_path.read_text(encoding="utf-8"))
            completed = next(
                line
                for line in lines
                if line["event"] == "intent_request_completed" and line["request_id"] == first["request_id"]
            )
            self.assertEqual(completed["session_id"], first_request["session_id"])
            self.assertEqual(completed["intent"], "project_3d_model")
            self.assertEqual(completed["confidence"], first["confidence"])
            self.assertEqual(completed["execution_policy"], "AUTO_EXECUTE")
            self.assertGreaterEqual(completed["duration_ms"], 0.0)
            self.assertEqual(completed["result"], "success")
            self.assertIsNone(completed["error_code"])

    async def test_single_engine_failures_degrade_and_dual_failure_rejects(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            base = Path(directory)
            model_down = IntentService.from_repository(
                self.root, model=UnavailableIntentModel(), audit_path=base / "model-down.jsonl"
            )
            rules_down = IntentService.from_repository(
                self.root, rule_engine=UnavailableRuleEngine(), audit_path=base / "rules-down.jsonl"
            )
            both_down = IntentService.from_repository(
                self.root,
                rule_engine=UnavailableRuleEngine(),
                model=UnavailableIntentModel(),
                audit_path=base / "both-down.jsonl",
            )

            rule_fallback = await model_down.parse(self._request("把设备模型投到桌面上"))
            model_fallback = await rules_down.parse(self._request("显示设备三维图"))
            rejected = await both_down.parse(self._request("把设备模型投到桌面上"))

            self.assertTrue(rule_fallback["processing"]["fallback_used"])
            self.assertEqual(rule_fallback["intent"], "project_3d_model")
            self.assertTrue(model_fallback["processing"]["fallback_used"])
            self.assertEqual(model_fallback["intent"], "project_3d_model")
            self.assertEqual(rejected["execution_policy"], "REJECT")
            self.assertEqual(rejected["error"]["code"], 2004)


if __name__ == "__main__":
    unittest.main()
