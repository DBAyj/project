from __future__ import annotations

from pathlib import Path
from datetime import datetime, timedelta, timezone
import unittest
from uuid import uuid4

from jsonschema import Draft202012Validator, FormatChecker

from astra_intent.application.intent_service import IntentService
from astra_intent.domain.errors import IntentServiceError
from astra_intent.infrastructure.clock import SystemClock


class AdjustableClock(SystemClock):
    def __init__(self) -> None:
        self.current = datetime(2026, 7, 14, 13, 40, tzinfo=timezone.utc)

    def now(self) -> datetime:
        return self.current


class IntentServiceIntegrationTests(unittest.IsolatedAsyncioTestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.root = Path(__file__).resolve().parents[4]
        cls.result_schema = __import__("json").loads(
            (cls.root / "protocols/intent/intent-result-v2.schema.json").read_text(encoding="utf-8")
        )

    def setUp(self) -> None:
        self.service = IntentService.from_repository(self.root)

    def _request(
        self,
        text: str,
        *,
        privacy: str = "PUBLIC",
        projection_state: str = "IDLE",
        locale: str = "zh-CN",
    ) -> dict[str, object]:
        return {
            "schema_version": "2.0",
            "request_id": str(uuid4()),
            "session_id": str(uuid4()),
            "user_id": "local-user",
            "locale": locale,
            "raw_text": text,
            "current_context": {
                "projection_state": projection_state,
                "current_space": "development_workspace",
                "current_privacy_level": privacy,
                "current_model_id": "demo-device",
            },
            "client": {"name": "astra-shell", "version": "0.2.0-alpha.1"},
            "requested_at": "2026-07-14T13:40:00Z",
        }

    def _assert_result_schema(self, value: dict[str, object]) -> None:
        errors = list(Draft202012Validator(self.result_schema, format_checker=FormatChecker()).iter_errors(value))
        self.assertEqual(errors, [], [error.message for error in errors])

    async def test_high_confidence_projection_is_auto_executable_and_protocol_valid(self) -> None:
        result = await self.service.parse(self._request("把设备模型投到桌面上"))

        self._assert_result_schema(result)
        self.assertEqual(result["intent"], "project_3d_model")
        self.assertGreaterEqual(result["confidence"], 0.85)
        self.assertEqual(result["execution_policy"], "AUTO_EXECUTE")
        self.assertEqual(result["slots"]["target_space"], "desk")
        self.assertEqual(result["slots"]["model_id"], "demo-device")

    async def test_unknown_and_ambiguous_requests_never_auto_execute(self) -> None:
        unknown = await self.service.parse(self._request("帮我买一张明天去上海的机票"))
        ambiguous = await self.service.parse(self._request("打开然后关闭投影"))
        missing = await self.service.parse(self._request("把那个放到那里"))

        self._assert_result_schema(unknown)
        self._assert_result_schema(ambiguous)
        self.assertEqual((unknown["intent"], unknown["execution_policy"], unknown["error"]["code"]), ("unknown", "REJECT", 2001))
        self.assertTrue(ambiguous["ambiguous"])
        self.assertEqual(ambiguous["execution_policy"], "ASK_CLARIFICATION")
        self.assertTrue(missing["ambiguous"])
        self.assertEqual(missing["execution_policy"], "ASK_CLARIFICATION")
        self.assertIn("模型", missing["clarification"]["question"])

    async def test_privacy_upgrade_requires_single_use_confirmation(self) -> None:
        result = await self.service.parse(self._request("公开展示", privacy="PRIVATE_SCREEN_ONLY"))

        self._assert_result_schema(result)
        self.assertEqual(result["execution_policy"], "REQUIRE_CONFIRMATION")
        confirmation_id = result["confirmation"]["confirmation_id"]
        accepted = self.service.confirm(confirmation_id)
        self.assertEqual(accepted["status"], "ACCEPTED")
        with self.assertRaises(IntentServiceError) as replay:
            self.service.confirm(confirmation_id)
        self.assertEqual(replay.exception.code, 2102)

    async def test_reject_marks_confirmation_and_prevents_reuse(self) -> None:
        result = await self.service.parse(self._request("停止投影", projection_state="ACTIVE"))
        confirmation_id = result["confirmation"]["confirmation_id"]

        rejected = self.service.reject(confirmation_id)

        self.assertEqual(rejected["status"], "REJECTED")
        with self.assertRaises(IntentServiceError):
            self.service.confirm(confirmation_id)

    async def test_expired_confirmation_is_rejected(self) -> None:
        clock = AdjustableClock()
        service = IntentService.from_repository(self.root, clock=clock)
        result = await service.parse(self._request("停止投影", projection_state="ACTIVE"))
        clock.current += timedelta(seconds=31)

        with self.assertRaises(IntentServiceError) as expired:
            service.confirm(result["confirmation"]["confirmation_id"])

        self.assertEqual(expired.exception.code, 2102)

    def test_health_readiness_and_supported_intents_are_real(self) -> None:
        self.assertEqual(self.service.health()["status"], "healthy")
        self.assertTrue(self.service.ready()["ready"])
        intents = self.service.supported_intents()
        self.assertEqual(len(intents), 30)
        self.assertIn("project_3d_model", intents)
        self.assertIn("unknown", intents)


if __name__ == "__main__":
    unittest.main()
