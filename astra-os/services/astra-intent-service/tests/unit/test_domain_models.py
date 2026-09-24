from __future__ import annotations

import unittest

from astra_intent.domain.confirmation import ConfirmationRequest, ConfirmationStatus
from astra_intent.domain.intent import IntentContext, IntentRequest, IntentSlots
from astra_intent.domain.intent_candidate import CandidateSource, IntentCandidate
from astra_intent.domain.intent_result import ExecutionPolicy, IntentResult, ProcessingEvidence


class IntentDomainModelTests(unittest.TestCase):
    def test_request_round_trip_preserves_typed_context(self) -> None:
        value = {
            "schema_version": "2.0",
            "request_id": "7abf7b1e-dc4f-4b7c-96cc-c3dd628e1ca9",
            "session_id": "98b998ba-e9e5-49f7-af4d-ce85d91a685c",
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
        request = IntentRequest.from_dict(value)
        self.assertIsInstance(request.context, IntentContext)
        self.assertEqual(request.context.projection_state, "IDLE")
        self.assertEqual(request.to_dict(), value)

    def test_result_serialization_is_protocol_shaped(self) -> None:
        candidate = IntentCandidate("project_3d_model", 0.97, CandidateSource.RULE_ENGINE, rule_id="project-model")
        result = IntentResult(
            request_id="7abf7b1e-dc4f-4b7c-96cc-c3dd628e1ca9",
            trace_id="bf489ffc-6bd5-464d-a1b7-5655863cb8c2",
            intent="project_3d_model",
            confidence=0.97,
            execution_policy=ExecutionPolicy.AUTO_EXECUTE,
            candidates=(candidate,),
            slots=IntentSlots(target_space="desk", model_id="demo-device", privacy_level="PUBLIC", display_target="projection_screen"),
            normalized_text="把设备模型投到桌面上",
            processing=ProcessingEvidence(rule_engine_used=True, local_model_used=True, duration_ms=18.0),
            created_at="2026-07-14T13:40:00Z",
        )
        value = result.to_dict()
        self.assertEqual(value["schema_version"], "2.0")
        self.assertEqual(value["execution_policy"], "AUTO_EXECUTE")
        self.assertEqual(value["candidates"][0]["source"], "RULE_ENGINE")
        self.assertFalse(value["requires_confirmation"])
        self.assertIsNone(value["error"])

    def test_confirmation_is_single_state_value(self) -> None:
        confirmation = ConfirmationRequest(
            confirmation_id="bf489ffc-6bd5-464d-a1b7-5655863cb8c2",
            request_id="7abf7b1e-dc4f-4b7c-96cc-c3dd628e1ca9",
            action="set_privacy_public",
            message="该操作会将内容设为公开投影，是否继续？",
            created_at="2026-07-14T13:40:00Z",
            expires_at="2026-07-14T13:40:30Z",
        )
        self.assertEqual(confirmation.status, ConfirmationStatus.PENDING)
        self.assertEqual(confirmation.to_dict()["status"], "PENDING")


if __name__ == "__main__":
    unittest.main()
