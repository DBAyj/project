from __future__ import annotations

from copy import deepcopy
import json
from pathlib import Path
import unittest

import jsonschema
import yaml


ROOT = Path(__file__).resolve().parents[2]
NOW = "2026-07-15T12:00:00Z"
REQUEST_ID = "7abf7b1e-dc4f-4b7c-96cc-c3dd628e1ca9"
SESSION_ID = "98b998ba-e9e5-49f7-af4d-ce85d91a685c"
LAYER_ID = "c38c0e21-96f1-4975-8cf3-fbfaa58c7cda"
GRAPH_ID = "acfe1ec3-18f5-4489-88bc-582c2a2b6274"
WARP_ID = "7d3c6f7c-28e4-40ec-89b4-23d36d1aa0e3"
MASK_ID = "6e20d0bc-b2d5-4407-bb1a-5c7c1f4f17c4"
P3_LIMITATION = "P3_REAL_SPATIAL_INTEGRATION_NOT_VERIFIED"
P3_VERIFIED = "P3_SERVICE_PROJECTION_TARGET_VERIFIED"


class P4SchemaContractTests(unittest.TestCase):
    def schema(self, relative: str) -> dict[str, object]:
        return json.loads((ROOT / relative).read_text(encoding="utf-8"))

    def assert_valid(self, relative: str, value: object) -> None:
        schema = self.schema(relative)
        validator_type = jsonschema.validators.validator_for(schema)
        validator_type.check_schema(schema)
        validator_type(schema, format_checker=jsonschema.FormatChecker()).validate(value)

    def assert_invalid(self, relative: str, value: object) -> None:
        with self.assertRaises(jsonschema.ValidationError):
            self.assert_valid(relative, value)

    def fixtures(self) -> dict[str, dict[str, object]]:
        return {
            "protocols/projection/projection-render-request-v1.schema.json": {
                "version": "1.0",
                "request_id": REQUEST_ID,
                "trace_id": REQUEST_ID,
                "session_id": SESSION_ID,
                "output_id": "window-projection",
                "privacy_level": "PUBLIC",
                "fixture_id": "front-rectangle",
                "layer_ids": [LAYER_ID],
                "p3_integration_status": P3_LIMITATION,
                "submitted_at": NOW,
            },
            "protocols/projection/projection-output-v1.schema.json": {
                "version": "1.0",
                "output_id": "window-projection",
                "output_type": "WINDOW",
                "state": "READY",
                "resolution": {"width": 1280, "height": 720},
                "refresh_rate_hz": 60.0,
                "updated_at": NOW,
            },
            "schemas/protocols/methods/projection-output-frame.result.schema.json": {
                "output_id": "window-projection",
                "frame_id": 1,
                "frame_png_base64": "iVBORw0KGgo=",
                "source": "P5_SPATIAL_UI",
                "public_labels": ["Public task"],
                "p3_integration_status": P3_LIMITATION,
            },
            "schemas/protocols/methods/projection-layers-submit.params.schema.json": {
                "request_id": REQUEST_ID, "trace_id": REQUEST_ID, "session_id": SESSION_ID,
                "output_id": "p4-window-projection", "fixture_id": "front-rectangle",
                "layers": [{"layer_id": LAYER_ID, "layer_type": "APPLICATION_SURFACE", "privacy_level": "PUBLIC",
                            "policy_decision_id": "a2e22408-67f8-5c77-aff7-a3b993ad1453",
                            "policy_subject_id": LAYER_ID,
                            "visible": True, "z_index": 1,
                            "bounds": {"x": 20, "y": 30, "width": 320, "height": 180}, "public_label": "Public task"}],
                "p3_integration_status": P3_LIMITATION,
            },
            "schemas/protocols/methods/projection-layers-submit.result.schema.json": {
                "session_id": SESSION_ID, "state": "RENDERING", "frame_id": 2,
                "accepted_layer_count": 1, "privacy_mask_final": True,
            },
            "protocols/projection/projection-runtime-session-v1.schema.json": {
                "version": "1.0",
                "session_id": SESSION_ID,
                "state": "READY",
                "output_id": "window-projection",
                "privacy_level": "PUBLIC",
                "p3_integration_status": P3_LIMITATION,
                "updated_at": NOW,
            },
            "protocols/projection/projection-layer-v1.schema.json": {
                "version": "1.0",
                "layer_id": LAYER_ID,
                "layer_type": "SCENE_3D",
                "privacy_level": "PUBLIC",
                "visible": True,
                "z_index": 1,
                "policy_decision_id": "a2e22408-67f8-5c77-aff7-a3b993ad1453",
                "policy_subject_id": LAYER_ID,
            },
            "protocols/projection/render-graph-v1.schema.json": {
                "version": "1.0",
                "graph_id": GRAPH_ID,
                "passes": [
                    {"pass_id": "InputPass", "depends_on": []},
                    {"pass_id": "OutputPass", "depends_on": ["InputPass"]},
                ],
            },
            "protocols/projection/geometry-warp-v1.schema.json": {
                "version": "1.0",
                "warp_id": WARP_ID,
                "fixture_id": "front-rectangle",
                "source_resolution": {"width": 1280, "height": 720},
                "destination_resolution": {"width": 1280, "height": 720},
                "corners": [
                    {"x": 0.0, "y": 0.0},
                    {"x": 1279.0, "y": 0.0},
                    {"x": 1279.0, "y": 719.0},
                    {"x": 0.0, "y": 719.0},
                ],
                "p3_integration_status": P3_LIMITATION,
            },
            "protocols/projection/privacy-mask-v1.schema.json": {
                "version": "1.0",
                "mask_id": MASK_ID,
                "mode": "RECTANGLE_MASK",
                "privacy_level": "ROOM_ONLY",
                "color": "#000000",
                "rectangle": {"x": 10, "y": 20, "width": 30, "height": 40},
            },
            "protocols/projection/projection-audit-event-v1.schema.json": {
                "version": "1.0",
                "event_id": REQUEST_ID,
                "timestamp": NOW,
                "event": "projection_safe_cleared",
                "trace_id": REQUEST_ID,
                "session_id": SESSION_ID,
                "result": "success",
                "error_code": None,
                "p3_integration_status": P3_LIMITATION,
            },
        }

    def test_public_p4_schemas_are_strict_and_fixture_only(self) -> None:
        for relative, payload in self.fixtures().items():
            with self.subTest(schema=relative):
                self.assert_valid(relative, payload)
                self.assert_invalid(relative, {**payload, "unknown": True})
                missing = deepcopy(payload)
                missing.pop(next(iter(payload)))
                self.assert_invalid(relative, missing)

    def test_geometry_and_privacy_contracts_reject_invalid_values(self) -> None:
        fixtures = self.fixtures()
        geometry_path = "protocols/projection/geometry-warp-v1.schema.json"
        privacy_path = "protocols/projection/privacy-mask-v1.schema.json"
        self.assert_invalid(geometry_path, {**fixtures[geometry_path], "fixture_id": "live-camera"})
        self.assert_invalid(geometry_path, {**fixtures[geometry_path], "corners": fixtures[geometry_path]["corners"][:3]})
        self.assert_invalid(privacy_path, {**fixtures[privacy_path], "color": "blue"})
        self.assert_invalid(privacy_path, {**fixtures[privacy_path], "mode": "ALPHA_MASK"})

    def test_p5_layer_submit_accepts_only_a_bound_verified_p3_target(self) -> None:
        params_path = "schemas/protocols/methods/projection-layers-submit.params.schema.json"
        frame_path = "schemas/protocols/methods/projection-output-frame.result.schema.json"
        params = deepcopy(self.fixtures()[params_path])
        params["p3_integration_status"] = P3_VERIFIED
        params["spatial_target"] = {
            "service": "astra-spatial-service", "input_source": "SIMULATION", "state": "TRACKING",
            "target": {
                "target_id": REQUEST_ID, "surface_id": SESSION_ID, "state": "CALIBRATED", "selected": True,
                "quality": "GOOD", "privacy_level": "NO_PROJECTION", "coordinate_system": "IMAGE_PIXEL",
                "corners": [
                    {"x": 10.0, "y": 10.0, "coordinate_system": "IMAGE_PIXEL"},
                    {"x": 620.0, "y": 10.0, "coordinate_system": "IMAGE_PIXEL"},
                    {"x": 620.0, "y": 470.0, "coordinate_system": "IMAGE_PIXEL"},
                    {"x": 10.0, "y": 470.0, "coordinate_system": "IMAGE_PIXEL"},
                ],
            },
        }
        self.assert_valid(params_path, params)
        missing_target = deepcopy(params)
        missing_target.pop("spatial_target")
        self.assert_invalid(params_path, missing_target)

        frame = {**self.fixtures()[frame_path], "p3_integration_status": P3_VERIFIED,
                 "spatial_target_id": REQUEST_ID}
        self.assert_valid(frame_path, frame)

    def test_p4_error_codes_are_registered_and_auditable(self) -> None:
        registry = yaml.safe_load((ROOT / "protocols/error-codes.yaml").read_text(encoding="utf-8"))
        entries = {entry["code"]: entry for entry in registry["codes"]}
        expected = {
            4006: "PROJECTION_OUTPUT_INITIALIZATION_FAILED",
            4007: "PROJECTION_OUTPUT_DISCONNECTED",
            4008: "RENDER_GRAPH_DEPENDENCY_MISSING",
            4009: "RENDER_GRAPH_CYCLE_DETECTED",
            4010: "PROJECTION_RENDER_PASS_FAILED",
            4011: "PROJECTION_HOMOGRAPHY_INVALID",
            4012: "PROJECTION_HOMOGRAPHY_NON_INVERTIBLE",
            4013: "PROJECTION_HOMOGRAPHY_OUT_OF_BOUNDS",
            4014: "PROJECTION_MESH_WARP_INVALID",
            4015: "PROJECTION_PRIVACY_MASK_INVALID",
            4016: "PROJECTION_SAFE_CLEAR_FAILED",
            4017: "PROJECTION_RENDER_TARGET_LOST",
            4018: "PROJECTION_OUTPUT_PRESENT_FAILED",
        }
        for code, name in expected.items():
            with self.subTest(code=code):
                self.assertIn(code, entries)
                self.assertEqual(entries[code]["name"], name)
                self.assertEqual(entries[code]["module"], "astra-projection")
                self.assertTrue(entries[code]["message"])

    def test_p4_methods_and_events_reference_strict_schemas(self) -> None:
        methods = yaml.safe_load((ROOT / "protocols/method-registry.yaml").read_text(encoding="utf-8"))["methods"]
        p4_methods = {entry["method"]: entry for entry in methods if entry["method"].startswith("projection.")}
        self.assertTrue({"projection.render", "projection.layers.submit", "projection.output.clear", "projection.session.command"}.issubset(p4_methods))
        for entry in p4_methods.values():
            for key in ("params_schema", "result_schema"):
                schema = self.schema(entry[key])
                self.assertFalse(schema.get("additionalProperties", True), entry["method"])

        events = yaml.safe_load((ROOT / "protocols/event-registry.yaml").read_text(encoding="utf-8"))["events"]
        p4_events = {entry["event"]: entry for entry in events if entry["event"].startswith("projection.")}
        self.assertTrue({"projection.render_state_changed", "projection.safe_cleared"}.issubset(p4_events))
        for entry in p4_events.values():
            schema = self.schema(entry["payload_schema"])
            self.assertFalse(schema.get("additionalProperties", True), entry["event"])


if __name__ == "__main__":
    unittest.main()
