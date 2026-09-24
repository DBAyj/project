from __future__ import annotations

import json
import unittest
from pathlib import Path

import jsonschema
import yaml


ROOT = Path(__file__).resolve().parents[2]


class P1SchemaContractTests(unittest.TestCase):
    def schema(self, relative_path: str) -> dict[object, object]:
        return json.loads((ROOT / relative_path).read_text(encoding="utf-8"))

    def assert_valid(self, relative_path: str, instance: object) -> None:
        schema = self.schema(relative_path)
        validator_type = jsonschema.validators.validator_for(schema)
        validator_type.check_schema(schema)
        validator_type(schema, format_checker=jsonschema.FormatChecker()).validate(instance)

    def assert_invalid(self, relative_path: str, instance: object) -> None:
        with self.assertRaises(jsonschema.ValidationError):
            self.assert_valid(relative_path, instance)

    def test_intent_contract_rejects_malformed_data(self) -> None:
        payload = {
            "version": "1.0",
            "request_id": "7abf7b1e-dc4f-4b7c-96cc-c3dd628e1ca9",
            "raw_text": "把设备模型投到桌面上",
            "intent": "project_3d_model",
            "confidence": 1.0,
            "target_space": "desk",
            "privacy_level": "PUBLIC",
            "parameters": {"model_id": "demo-device"},
            "timestamp": "2026-07-14T08:30:00Z",
        }
        self.assert_valid("protocols/intent/intent-v1.schema.json", payload)
        self.assert_invalid("protocols/intent/intent-v1.schema.json", {**payload, "extra": True})
        self.assert_invalid("protocols/intent/intent-v1.schema.json", {**payload, "request_id": "not-a-uuid"})
        self.assert_invalid("protocols/intent/intent-v1.schema.json", {**payload, "timestamp": "yesterday"})

    def test_projection_session_contract_rejects_unknown_state(self) -> None:
        payload = {
            "version": "1.0",
            "session_id": "7abf7b1e-dc4f-4b7c-96cc-c3dd628e1ca9",
            "state": "ACTIVE",
            "target_space": "desk",
            "privacy_level": "ROOM_ONLY",
            "model_id": "demo-device",
            "updated_at": "2026-07-14T08:30:00Z",
        }
        self.assert_valid("protocols/projection/projection-session-v1.schema.json", payload)
        self.assert_invalid("protocols/projection/projection-session-v1.schema.json", {**payload, "state": "RUNNING"})
        self.assert_invalid("protocols/projection/projection-session-v1.schema.json", {key: value for key, value in payload.items() if key != "model_id"})

    def test_audit_event_contract_rejects_bad_enums_and_identifiers(self) -> None:
        payload = {
            "schema_version": "1.0",
            "timestamp": "2026-07-14T08:30:00Z",
            "level": "INFO",
            "service": "astra-shell",
            "module": "projection.session",
            "event": "projection_started",
            "trace_id": "7abf7b1e-dc4f-4b7c-96cc-c3dd628e1ca9",
            "request_id": "7abf7b1e-dc4f-4b7c-96cc-c3dd628e1ca9",
            "session_id": "7abf7b1e-dc4f-4b7c-96cc-c3dd628e1ca9",
            "actor": "local-user",
            "intent": "project_3d_model",
            "privacy_level": "PUBLIC",
            "result": "success",
            "error_code": None,
            "message": "Projection session started",
            "details": {"target_space": "desk"},
        }
        self.assert_valid("protocols/audit/audit-event-v1.schema.json", payload)
        self.assert_invalid("protocols/audit/audit-event-v1.schema.json", {**payload, "level": "NOTICE"})
        self.assert_invalid("protocols/audit/audit-event-v1.schema.json", {**payload, "trace_id": "not-a-uuid"})
        self.assert_invalid("protocols/audit/audit-event-v1.schema.json", {**payload, "timestamp": "not-a-time"})

    def test_configuration_and_privacy_policy_contracts_reject_unknown_fields(self) -> None:
        configuration = yaml.safe_load((ROOT / "config/astra.example.yaml").read_text(encoding="utf-8"))
        self.assert_valid("schemas/application-config.schema.json", configuration)
        self.assert_invalid("schemas/application-config.schema.json", {**configuration, "unexpected": True})
        self.assert_invalid(
            "schemas/application-config.schema.json",
            {**configuration, "projection": {**configuration["projection"], "default_target": "ceiling"}},
        )

        privacy_policy = {
            "version": "1.0",
            "privacy_level": "AUTHORIZED_PERSON",
            "projection_allowed": False,
            "reason": "authorization_required",
        }
        self.assert_valid("schemas/privacy-policy.schema.json", privacy_policy)
        self.assert_invalid("schemas/privacy-policy.schema.json", {**privacy_policy, "privacy_level": "SECRET"})
        self.assert_invalid("schemas/privacy-policy.schema.json", {**privacy_policy, "extra": True})


if __name__ == "__main__":
    unittest.main()
