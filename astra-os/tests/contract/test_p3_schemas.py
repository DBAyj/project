from __future__ import annotations

from copy import deepcopy
import json
from pathlib import Path
import unittest

import jsonschema
import yaml


ROOT = Path(__file__).resolve().parents[2]
UUID = "7abf7b1e-dc4f-4b7c-96cc-c3dd628e1ca9"
UUID_2 = "98b998ba-e9e5-49f7-af4d-ce85d91a685c"
NOW = "2026-07-15T00:00:00Z"
POINTS = [
    {"x": 100.0, "y": 200.0, "coordinate_system": "IMAGE_PIXEL"},
    {"x": 900.0, "y": 180.0, "coordinate_system": "IMAGE_PIXEL"},
    {"x": 950.0, "y": 650.0, "coordinate_system": "IMAGE_PIXEL"},
    {"x": 120.0, "y": 670.0, "coordinate_system": "IMAGE_PIXEL"},
]


class P3SchemaContractTests(unittest.TestCase):
    def assert_valid(self, relative: str, value: object) -> None:
        schema = json.loads((ROOT / relative).read_text(encoding="utf-8"))
        validator_type = jsonschema.validators.validator_for(schema)
        validator_type.check_schema(schema)
        validator_type(schema, format_checker=jsonschema.FormatChecker()).validate(value)

    def assert_invalid(self, relative: str, value: object) -> None:
        with self.assertRaises(jsonschema.ValidationError):
            self.assert_valid(relative, value)

    def fixtures(self) -> dict[str, dict[str, object]]:
        matrix = [1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0]
        transform = [1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0]
        return {
            "protocols/spatial/spatial-frame-v1.schema.json": {
                "schema_version": "1.0", "frame_id": UUID, "source": "SIMULATION", "width": 1280,
                "height": 720, "pixel_format": "BGR8", "coordinate_system": "IMAGE_PIXEL", "timestamp": NOW,
            },
            "protocols/spatial/surface-candidate-v1.schema.json": {
                "schema_version": "1.0", "surface_id": UUID, "surface_type": "DESK", "confidence": 0.88,
                "corners": POINTS, "area_ratio": 0.48, "aspect_ratio": 1.72, "stability_score": 0.91,
                "score": 0.87, "quality": "GOOD", "coordinate_system": "IMAGE_PIXEL",
            },
            "protocols/spatial/spatial-state-v1.schema.json": {
                "schema_version": "1.0", "state": "TRACKING", "input_source": "SIMULATION",
                "camera_status": "INACTIVE", "frame_id": UUID, "frame_timestamp": NOW,
                "surface_candidates": 1, "selected_target_id": UUID_2, "scene_id": UUID,
                "quality": "GOOD", "observer_tracking": "SIMULATED", "processing_duration_ms": 12.0,
                "coordinate_system": "WORLD_SIMULATED", "warnings": [],
            },
            "protocols/spatial/spatial-anchor-v1.schema.json": {
                "schema_version": "1.0", "anchor_id": UUID, "anchor_type": "PROJECTION_SURFACE",
                "coordinate_system": "SURFACE_LOCAL", "transform": transform, "quality": "GOOD",
                "persistent": False, "created_at": NOW, "last_updated_at": NOW,
            },
            "protocols/spatial/scene-object-v1.schema.json": {
                "schema_version": "1.0", "object_id": UUID, "scene_id": UUID, "object_type": "MODEL", "model_id": "demo-device",
                "anchor_id": UUID_2, "coordinate_system": "SURFACE_LOCAL",
                "position": {"x": 0.5, "y": 0.5, "z": 0.0}, "rotation": {"x": 0.0, "y": 0.0, "z": 0.0},
                "scale": {"x": 1.0, "y": 1.0, "z": 1.0}, "visibility": True, "privacy_level": "PUBLIC",
                "interaction_enabled": True, "lifecycle_state": "ACTIVE",
            },
            "protocols/spatial/projection-target-v1.schema.json": {
                "schema_version": "1.0", "target_id": UUID, "surface_id": UUID_2, "surface_type": "DESK",
                "source_resolution": {"width": 1280, "height": 720}, "target_resolution": {"width": 1280, "height": 720},
                "corners": POINTS, "coordinate_system": "IMAGE_PIXEL", "target_coordinate_system": "SURFACE_LOCAL",
                "homography": matrix, "inverse_homography": matrix, "quality": "GOOD", "privacy_level": "NO_PROJECTION", "state": "CALIBRATED",
                "selected": True, "created_at": NOW, "updated_at": NOW,
            },
            "protocols/spatial/spatial-event-v1.schema.json": {
                "schema_version": "1.0", "event_id": UUID, "timestamp": NOW, "service": "astra-spatial-service",
                "event": "surface_selected", "trace_id": UUID_2, "request_id": UUID,
                "state": "TRACKING", "result": "success", "error_code": None,
                "coordinate_system": "IMAGE_PIXEL", "details": {},
            },
            "protocols/spatial/spatial-scene-v1.schema.json": {
                "schema_version": "1.0", "scene_id": UUID, "state": "TRACKING", "coordinate_system": "WORLD_SIMULATED",
                "anchor_ids": [UUID_2], "object_ids": [UUID], "created_at": NOW, "updated_at": NOW,
            },
            "protocols/spatial/observer-pose-v1.schema.json": {
                "schema_version": "1.0", "observer_id": "primary-observer", "position": {"x": 0.0, "y": 0.2, "z": 1.5},
                "orientation": {"x": 0.0, "y": 0.0, "z": 0.0}, "tracking_state": "SIMULATED", "confidence": 1.0,
                "coordinate_system": "WORLD_SIMULATED",
            },
        }

    def test_all_spatial_protocols_are_strict_and_coordinate_explicit(self) -> None:
        for relative, value in self.fixtures().items():
            with self.subTest(schema=relative):
                self.assert_valid(relative, value)
                self.assert_invalid(relative, {**value, "unknown": True})
                missing = deepcopy(value)
                missing.pop("coordinate_system")
                self.assert_invalid(relative, missing)
                self.assert_invalid(relative, {**value, "schema_version": "9.0"})

    def test_spatial_geometry_rejects_wrong_point_and_matrix_dimensions(self) -> None:
        fixtures = self.fixtures()
        candidate_path = "protocols/spatial/surface-candidate-v1.schema.json"
        target_path = "protocols/spatial/projection-target-v1.schema.json"
        self.assert_invalid(candidate_path, {**fixtures[candidate_path], "corners": POINTS[:3]})
        self.assert_invalid(target_path, {**fixtures[target_path], "homography": [1.0] * 8})
        self.assert_invalid(candidate_path, {**fixtures[candidate_path], "confidence": 1.1})

    def test_spatial_configuration_files_match_strict_schemas(self) -> None:
        pairs = (
            ("config/spatial.yaml", "schemas/spatial-config.schema.json"),
            ("config/camera.yaml", "schemas/camera-config.schema.json"),
            ("config/surface-detection.yaml", "schemas/surface-detection-config.schema.json"),
            ("config/calibration.yaml", "schemas/calibration-config.schema.json"),
        )
        for config_path, schema_path in pairs:
            with self.subTest(config=config_path):
                value = yaml.safe_load((ROOT / config_path).read_text(encoding="utf-8"))
                self.assert_valid(schema_path, value)
                self.assert_invalid(schema_path, {**value, "unknown": True})

    def test_calibration_profile_is_strict_and_coordinate_explicit(self) -> None:
        matrix = [1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0]
        profile = {
            "schema_version": "1.0", "profile_id": UUID, "profile_version": "1.0",
            "source_resolution": {"width": 1280, "height": 720}, "corners": POINTS,
            "coordinate_system": "IMAGE_PIXEL", "target_coordinate_system": "SURFACE_LOCAL",
            "homography": matrix, "inverse_homography": matrix, "reprojection_error_pixels": 0.0,
            "created_at": NOW,
        }
        path = "schemas/calibration-profile.schema.json"
        self.assert_valid(path, profile)
        self.assert_invalid(path, {**profile, "coordinate_system": "PHONE_VIEW"})
        self.assert_invalid(path, {**profile, "homography": matrix[:8]})

    def test_spatial_public_methods_and_events_have_strict_schemas(self) -> None:
        methods = yaml.safe_load((ROOT / "protocols/method-registry.yaml").read_text(encoding="utf-8"))["methods"]
        expected_methods = {
            "spatial.health", "spatial.state", "spatial.source.start", "spatial.source.stop", "spatial.detect",
            "spatial.select", "spatial.calibrate", "spatial.observer.set", "spatial.reset",
        }
        registered = {entry["method"] for entry in methods}
        self.assertTrue(expected_methods.issubset(registered))
        for entry in methods:
            if not entry["method"].startswith("spatial."):
                continue
            for key in ("params_schema", "result_schema"):
                schema = json.loads((ROOT / entry[key]).read_text(encoding="utf-8"))
                self.assertTrue(schema.get("additionalProperties") is False or "$ref" in schema, entry["method"])
        events = yaml.safe_load((ROOT / "protocols/event-registry.yaml").read_text(encoding="utf-8"))["events"]
        spatial_event = next(entry for entry in events if entry["event"] == "spatial.state_changed")
        schema = json.loads((ROOT / spatial_event["payload_schema"]).read_text(encoding="utf-8"))
        self.assertFalse(schema["additionalProperties"])

        calibrate_params = {
            "corners": POINTS,
        }
        calibrate_path = "schemas/protocols/methods/spatial-calibrate.params.schema.json"
        self.assert_valid(calibrate_path, calibrate_params)
        self.assert_invalid(calibrate_path, {})
        self.assert_invalid(calibrate_path, {"corners": POINTS[:3]})
        self.assert_invalid(calibrate_path, {"corners": [{"x": 1.0, "y": 2.0, "coordinate_system": "SURFACE_LOCAL"}] * 4})

    def test_p3_error_registry_entries_are_complete_and_auditable(self) -> None:
        registry = yaml.safe_load((ROOT / "protocols/error-codes.yaml").read_text(encoding="utf-8"))
        entries = [entry for entry in registry["codes"] if 3000 <= entry["code"] < 4000]
        expected_codes = {
            *range(3001, 3010),
            *range(3101, 3106),
            *range(3201, 3209),
            *range(3301, 3307),
            *range(3401, 3405),
            *range(3501, 3505),
        }
        self.assertEqual({entry["code"] for entry in entries}, expected_codes)
        for entry in entries:
            with self.subTest(code=entry["code"]):
                self.assertEqual(entry["module"], "astra-spatial")
                self.assertTrue(entry["message"])
                self.assertTrue(entry["message_zh"])
                self.assertIn(entry["severity"], {"WARNING", "ERROR", "CRITICAL"})
                self.assertIsInstance(entry["retryable"], bool)
                self.assertTrue(entry["audit_required"])


if __name__ == "__main__":
    unittest.main()
