from __future__ import annotations

import json
import unittest
from copy import deepcopy
from pathlib import Path

import jsonschema
import yaml


ROOT = Path(__file__).resolve().parents[2]


class P2SchemaContractTests(unittest.TestCase):
    def schema(self, relative_path: str) -> dict[str, object]:
        return json.loads((ROOT / relative_path).read_text(encoding="utf-8"))

    def assert_valid(self, relative_path: str, value: object) -> None:
        schema = self.schema(relative_path)
        validator_type = jsonschema.validators.validator_for(schema)
        validator_type.check_schema(schema)
        validator_type(schema, format_checker=jsonschema.FormatChecker()).validate(value)

    def assert_invalid(self, relative_path: str, value: object) -> None:
        with self.assertRaises(jsonschema.ValidationError):
            self.assert_valid(relative_path, value)

    def request(self) -> dict[str, object]:
        return {
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

    def result(self) -> dict[str, object]:
        return {
            "schema_version": "2.0",
            "request_id": "7abf7b1e-dc4f-4b7c-96cc-c3dd628e1ca9",
            "trace_id": "bf489ffc-6bd5-464d-a1b7-5655863cb8c2",
            "intent": "project_3d_model",
            "confidence": 0.97,
            "execution_policy": "AUTO_EXECUTE",
            "requires_confirmation": False,
            "ambiguous": False,
            "candidates": [{"intent": "project_3d_model", "confidence": 0.97, "source": "RULE_ENGINE"}],
            "slots": {
                "target_space": "desk",
                "model_id": "demo-device",
                "privacy_level": "PUBLIC",
                "zoom_factor": None,
                "rotation_direction": None,
                "rotation_degrees": None,
                "display_target": "projection_screen",
                "confirmation_response": None,
            },
            "normalized_text": "把设备模型投到桌面上",
            "processing": {
                "rule_engine_used": True,
                "local_model_used": True,
                "fallback_used": False,
                "cache_hit": False,
                "duration_ms": 18.0,
            },
            "warnings": [],
            "error": None,
            "clarification": None,
            "confirmation": None,
            "created_at": "2026-07-14T13:40:00Z",
        }

    def test_intent_request_v2_is_strict(self) -> None:
        path = "protocols/intent/intent-request-v2.schema.json"
        value = self.request()
        self.assert_valid(path, value)
        self.assert_invalid(path, {**value, "extra": True})
        missing = deepcopy(value)
        missing.pop("raw_text")
        self.assert_invalid(path, missing)
        self.assert_invalid(path, {**value, "request_id": "bad"})
        self.assert_invalid(path, {**value, "locale": "fr-FR"})
        self.assert_invalid(path, {**value, "requested_at": "tomorrow"})
        self.assert_invalid(path, {**value, "raw_text": "x" * 2001})

    def test_intent_result_v2_is_strict(self) -> None:
        path = "protocols/intent/intent-result-v2.schema.json"
        value = self.result()
        self.assert_valid(path, value)
        self.assert_invalid(path, {**value, "extra": True})
        self.assert_invalid(path, {**value, "confidence": 1.1})
        self.assert_invalid(path, {**value, "execution_policy": "RUN"})
        self.assert_invalid(path, {**value, "trace_id": "bad"})
        self.assert_invalid(path, {**value, "created_at": "now"})

    def test_candidate_confirmation_and_event_contracts_are_strict(self) -> None:
        candidate = {"schema_version": "1.0", "intent": "stop_projection", "confidence": 1.0, "source": "RULE_ENGINE"}
        self.assert_valid("protocols/intent/intent-candidate-v1.schema.json", candidate)
        self.assert_invalid("protocols/intent/intent-candidate-v1.schema.json", {**candidate, "source": "CLOUD"})
        self.assert_invalid("protocols/intent/intent-candidate-v1.schema.json", {**candidate, "confidence": -0.1})
        self.assert_invalid("protocols/intent/intent-candidate-v1.schema.json", {**candidate, "unknown": True})
        confirmation = {
            "schema_version": "1.0",
            "confirmation_id": "bf489ffc-6bd5-464d-a1b7-5655863cb8c2",
            "request_id": "7abf7b1e-dc4f-4b7c-96cc-c3dd628e1ca9",
            "action": "set_privacy_public",
            "message": "该操作会将内容设为公开投影，是否继续？",
            "status": "PENDING",
            "created_at": "2026-07-14T13:40:00Z",
            "expires_at": "2026-07-14T13:40:30Z",
        }
        self.assert_valid("protocols/intent/intent-confirmation-v1.schema.json", confirmation)
        self.assert_invalid("protocols/intent/intent-confirmation-v1.schema.json", {**confirmation, "status": "DONE"})
        self.assert_invalid("protocols/intent/intent-confirmation-v1.schema.json", {**confirmation, "confirmation_id": "bad"})
        self.assert_invalid("protocols/intent/intent-confirmation-v1.schema.json", {**confirmation, "unknown": True})
        event = {
            "schema_version": "1.0",
            "event_id": "5c3eac73-3f99-4383-be3d-6499d3f17689",
            "timestamp": "2026-07-14T13:40:00Z",
            "level": "INFO",
            "service": "astra-intent-service",
            "event": "intent_request_completed",
            "trace_id": "bf489ffc-6bd5-464d-a1b7-5655863cb8c2",
            "request_id": "7abf7b1e-dc4f-4b7c-96cc-c3dd628e1ca9",
            "session_id": "98b998ba-e9e5-49f7-af4d-ce85d91a685c",
            "intent": "project_3d_model",
            "confidence": 0.97,
            "execution_policy": "AUTO_EXECUTE",
            "duration_ms": 18.0,
            "result": "success",
            "error_code": None,
            "details": self.result(),
        }
        self.assert_valid("protocols/intent/intent-event-v1.schema.json", event)
        self.assert_invalid("protocols/intent/intent-event-v1.schema.json", {**event, "timestamp": "bad"})
        self.assert_invalid("protocols/intent/intent-event-v1.schema.json", {**event, "event_id": "bad"})
        self.assert_invalid("protocols/intent/intent-event-v1.schema.json", {**event, "unknown": True})
        for required in (
            "timestamp",
            "level",
            "service",
            "session_id",
            "intent",
            "confidence",
            "execution_policy",
            "duration_ms",
            "result",
            "error_code",
            "details",
        ):
            missing = deepcopy(event)
            missing.pop(required)
            self.assert_invalid("protocols/intent/intent-event-v1.schema.json", missing)

    def test_p2_configuration_files_match_strict_schemas(self) -> None:
        pairs = (
            ("config/intent.yaml", "schemas/intent-config.schema.json"),
            ("config/intent-rules.yaml", "schemas/intent-rules.schema.json"),
            ("config/intent-security.yaml", "schemas/intent-security.schema.json"),
            ("config/model-routing.yaml", "schemas/model-routing.schema.json"),
        )
        for config_path, schema_path in pairs:
            value = yaml.safe_load((ROOT / config_path).read_text(encoding="utf-8"))
            self.assert_valid(schema_path, value)
            self.assert_invalid(schema_path, {**value, "unknown": True})


if __name__ == "__main__":
    unittest.main()
