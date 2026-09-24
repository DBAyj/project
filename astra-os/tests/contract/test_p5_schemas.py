from __future__ import annotations

from copy import deepcopy
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

import jsonschema
import yaml

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))

from method_capabilities import method_capabilities


UUID = "7abf7b1e-dc4f-4b7c-96cc-c3dd628e1ca9"
UUID_2 = "c38c0e21-96f1-4975-8cf3-fbfaa58c7cda"
NOW = "2026-07-15T12:00:00Z"
MARKER = "P4_RELEASE_BASELINE_FINAL"


class P5SchemaContractTests(unittest.TestCase):
    def assert_valid(self, relative: str, value: object) -> None:
        schema = json.loads((ROOT / relative).read_text(encoding="utf-8"))
        validator_type = jsonschema.validators.validator_for(schema)
        validator_type.check_schema(schema)
        validator_type(schema, format_checker=jsonschema.FormatChecker()).validate(value)

    def fixtures(self) -> dict[str, dict[str, object]]:
        component = {
            "schema_version": "1.0", "component_id": UUID, "component_type": "TASK_CARD", "parent_id": None,
            "children": [], "bounds": {"x": 10.0, "y": 20.0, "width": 320.0, "height": 180.0},
            "transform": {"translation_x": 0.0, "translation_y": 0.0, "rotation_degrees": 0.0, "scale": 1.0},
            "z_order": 2, "opacity": 1.0, "visible": True, "enabled": True, "focusable": True,
            "interactive": True, "privacy_level": "PUBLIC", "accessibility_label": "Task card",
            "lifecycle_state": "VISIBLE", "created_at": NOW, "updated_at": NOW,
        }
        return {
            "protocols/spatial-ui/spatial-component-v1.schema.json": component,
            "protocols/spatial-ui/spatial-window-v1.schema.json": {
                "schema_version": "1.0", "window_id": UUID, "component_id": UUID_2, "state": "VISIBLE",
                "display_target": "PHONE", "projection_target": "fixture-front", "anchor_id": None,
                "privacy_level": "PUBLIC", "focus_scope": "workspace", "bounds": component["bounds"], "updated_at": NOW,
            },
            "protocols/spatial-ui/spatial-layout-v1.schema.json": {
                "schema_version": "1.0", "layout_id": UUID, "mode": "STACK", "direction": "VERTICAL",
                "component_ids": [UUID_2], "spacing": 16.0, "margin": 24.0,
                "safe_area": {"x": 0.0, "y": 0.0, "width": 1280.0, "height": 720.0}, "updated_at": NOW,
            },
            "protocols/spatial-ui/spatial-input-event-v1.schema.json": {
                "schema_version": "1.0", "event_id": UUID, "event_type": "POINTER_PRESS", "source_type": "MOUSE",
                "source_id": "primary-mouse", "target_component_id": UUID_2,
                "position": {"coordinate_system": "PROJECTION_VIEW", "x": 300.0, "y": 220.0},
                "modifiers": [], "timestamp": NOW,
            },
            "protocols/spatial-ui/spatial-focus-event-v1.schema.json": {
                "schema_version": "1.0", "event_id": UUID, "scope_id": "workspace", "previous_component_id": None,
                "component_id": UUID_2, "focus_type": "KEYBOARD", "priority": "ACTIVE_WINDOW", "reason": "tab", "timestamp": NOW,
            },
            "protocols/spatial-ui/spatial-notification-v1.schema.json": {
                "schema_version": "1.0", "notification_id": UUID, "title": "Projection ready", "message": "Fixture output active",
                "severity": "SUCCESS", "privacy_level": "PUBLIC", "display_target": "PHONE", "timeout_ms": 3000,
                "requires_action": False, "actions": [], "created_at": NOW, "expires_at": NOW,
            },
            "protocols/spatial-ui/spatial-ui-request-v1.schema.json": {
                "schema_version": "1.0", "request_id": UUID, "trace_id": UUID_2, "session_id": UUID,
                "action": "COMPONENT_CREATE", "actor": "local-user", "privacy_level": "PUBLIC", "payload": {}, "submitted_at": NOW,
                "p4_release_status": MARKER,
            },
            "protocols/spatial-ui/spatial-ui-state-v1.schema.json": {
                "schema_version": "1.0", "session_id": UUID, "status": "READY", "component_count": 1, "window_count": 1,
                "focus_component_id": UUID_2, "layout_mode": "STACK", "projection_safe": True, "updated_at": NOW,
                "p4_release_status": MARKER,
            },
            "protocols/spatial-ui/spatial-ui-event-v1.schema.json": {
                "schema_version": "1.0", "event_id": UUID, "event": "component_created", "timestamp": NOW,
                "trace_id": UUID_2, "request_id": UUID, "session_id": UUID, "actor": "local-user",
                "privacy_level": "PUBLIC", "result": "success", "error_code": None, "payload": {},
                "p4_release_status": MARKER,
            },
            "protocols/spatial-ui/ui-state-v1.schema.json": {
                "schema_version": "1.1", "layout_mode": "STACK", "components": [], "windows": [], "visible_component_ids": [],
                "selected_tab": "tasks", "focus_restore_component_id": None, "panel_order": ["tasks", "system"], "saved_at": NOW,
            },
            "schemas/spatial-ui-config.schema.json": {
                "service": {"name": "astra-spatial-ui-service", "version": "0.5.0-alpha.1"},
                "runtime": {"maximum_components": 500, "maximum_windows": 20, "state_persistence": True, "autosave_interval_seconds": 10},
                "display": {"default_target": "PHONE", "projection_target_enabled": True},
                "privacy": {"default_level": "PRIVATE_SCREEN_ONLY", "require_policy_check": True,
                            "public_fixture_subject_id": "7abf7b1e-dc4f-4b7c-96cc-c3dd628e1ca9"},
            },
            "schemas/spatial-layout-config.schema.json": {
                "layout": {"default_mode": "STACK", "spacing": 16, "margin": 24, "keep_inside_target": True, "avoid_overlap": True}
            },
            "schemas/spatial-input-config.schema.json": {
                "input": {"mouse_enabled": True, "keyboard_enabled": True, "touch_enabled": True,
                          "simulated_gesture_enabled": True, "double_click_interval_ms": 300, "drag_threshold_pixels": 6}
            },
            "schemas/spatial-accessibility-config.schema.json": {
                "accessibility": {"enabled": True, "keyboard_navigation": True, "visible_focus": True,
                                  "reduce_motion": False, "high_contrast": False}
            },
            "schemas/spatial-component-theme.schema.json": {
                "background": "#0d1117", "surface": "#1d2633", "text": "#f2f5f8", "accent": "#66a6ff",
                "ai_accent": "#b696ff", "projection_accent": "#4ed5b2", "privacy": "#f06c85", "success": "#52c78f",
                "warning": "#f2bb5c", "error": "#f06c85", "focus_outline": "#ffffff", "radius": 6,
                "shadow": 8, "spacing": 16, "animation_ms": 220, "font_scale": 1.0,
            },
        }

    def test_public_schemas_are_strict(self) -> None:
        for relative, payload in self.fixtures().items():
            with self.subTest(schema=relative):
                self.assert_valid(relative, payload)
                with self.assertRaises(jsonschema.ValidationError):
                    self.assert_valid(relative, {**payload, "unknown": True})
                missing = deepcopy(payload)
                missing.pop(next(iter(missing)))
                with self.assertRaises(jsonschema.ValidationError):
                    self.assert_valid(relative, missing)

    def test_component_and_input_reject_invalid_domain_values(self) -> None:
        fixtures = self.fixtures()
        component_path = "protocols/spatial-ui/spatial-component-v1.schema.json"
        input_path = "protocols/spatial-ui/spatial-input-event-v1.schema.json"
        for value in (
            {**fixtures[component_path], "component_id": "bad"},
            {**fixtures[component_path], "opacity": 1.1},
            {**fixtures[component_path], "privacy_level": "SECRET"},
            {**fixtures[component_path], "lifecycle_state": "RUNNING"},
        ):
            with self.assertRaises(jsonschema.ValidationError):
                self.assert_valid(component_path, value)
        with self.assertRaises(jsonschema.ValidationError):
            self.assert_valid(input_path, {**fixtures[input_path], "event_type": "CAMERA_GESTURE"})
        gesture = {
            **fixtures[input_path],
            "event_type": "GESTURE_SCALE",
            "source_type": "SIMULATED_GESTURE",
            "source_id": "fixture-gesture",
            "interaction_value": 1.25,
        }
        self.assert_valid(input_path, gesture)
        with self.assertRaises(jsonschema.ValidationError):
            missing_value = deepcopy(gesture)
            missing_value.pop("interaction_value")
            self.assert_valid(input_path, missing_value)
        with self.assertRaises(jsonschema.ValidationError):
            self.assert_valid(input_path, {**fixtures[input_path], "interaction_value": 1.25})
        action = {**fixtures[input_path], "event_type": "AI_ACTION", "source_type": "SYSTEM",
                  "source_id": "astra-shell", "target_component_id": UUID, "position": None,
                  "action": "Confirm"}
        self.assert_valid(input_path, action)
        with self.assertRaises(jsonschema.ValidationError):
            missing_action = deepcopy(action)
            missing_action.pop("action")
            self.assert_valid(input_path, missing_action)
        with self.assertRaises(jsonschema.ValidationError):
            self.assert_valid(input_path, {**action, "source_type": "MOUSE"})

    def test_p5_error_codes_are_registered(self) -> None:
        entries = {entry["code"] for entry in yaml.safe_load((ROOT / "protocols/error-codes.yaml").read_text(encoding="utf-8"))["codes"]}
        required = set(range(5101, 5108)) | set(range(5201, 5207)) | set(range(5301, 5306)) | set(range(5401, 5407))
        required |= set(range(5501, 5505)) | set(range(5601, 5606)) | set(range(5701, 5705)) | set(range(5801, 5805)) | set(range(5901, 5906))
        self.assertTrue(required.issubset(entries))

    def test_spatial_ui_rpc_registry_and_strict_parameters(self) -> None:
        registry = yaml.safe_load((ROOT / "protocols/method-registry.yaml").read_text(encoding="utf-8"))["methods"]
        methods = {entry["method"]: entry for entry in registry}
        required = {
            "spatial_ui.status", "spatial_ui.components", "spatial_ui.windows", "spatial_ui.focus.status",
            "spatial_ui.layout.status", "spatial_ui.metrics", "spatial_ui.component.create", "spatial_ui.component.update",
            "spatial_ui.task.create", "spatial_ui.task.update", "spatial_ui.interaction.cancel",
            "spatial_ui.component.remove", "spatial_ui.window.open", "spatial_ui.window.move", "spatial_ui.window.resize",
            "spatial_ui.window.hide", "spatial_ui.window.show", "spatial_ui.window.target", "spatial_ui.window.restore",
            "spatial_ui.window.close", "spatial_ui.layout.apply", "spatial_ui.layout.reset", "spatial_ui.input",
            "spatial_ui.focus", "spatial_ui.notification.create", "spatial_ui.notification.clear", "spatial_ui.state.save",
            "spatial_ui.state.load", "spatial_ui.target.lost", "spatial_ui.target.available", "spatial_ui.reset",
        }
        self.assertTrue(required.issubset(methods))
        paths: set[str] = set()
        for name in required:
            for key in ("params_schema", "result_schema"):
                relative = methods[name][key]
                self.assertNotIn(relative, paths, f"{name} must have an independent {key}")
                paths.add(relative)
                self.assertTrue((ROOT / relative).is_file())
                schema = json.loads((ROOT / relative).read_text(encoding="utf-8"))
                self.assertIs(schema.get("additionalProperties"), False)
                self.assertNotIn("oneOf", schema)
        create = {"component_id": UUID, "component_type": "TASK_CARD", "parent_id": None, "children": [],
                  "privacy_level": "PUBLIC", "accessibility_label": "Task",
                  "bounds": {"x": 0, "y": 0, "width": 320, "height": 180}, "display_target": "BOTH"}
        create_schema = methods["spatial_ui.component.create"]["params_schema"]
        self.assert_valid(create_schema, create)
        with self.assertRaises(jsonschema.ValidationError):
            self.assert_valid(create_schema, {**create, "component_id": "task-1"})
        with self.assertRaises(jsonschema.ValidationError):
            self.assert_valid(create_schema, {**create, "unknown": True})
        input_schema = methods["spatial_ui.input"]["params_schema"]
        rpc_input = self.fixtures()["protocols/spatial-ui/spatial-input-event-v1.schema.json"]
        self.assert_valid(input_schema, rpc_input)
        self.assert_valid(input_schema, {**rpc_input, "event_type": "GESTURE_ROTATE", "source_type": "SIMULATED_GESTURE",
                                         "source_id": "fixture-gesture", "interaction_value": 15.0})
        self.assert_valid(input_schema, {**rpc_input, "event_type": "AI_ACTION", "source_type": "SYSTEM",
                                         "source_id": "astra-shell", "target_component_id": UUID,
                                         "position": None, "action": "Confirm"})
        with self.assertRaises(jsonschema.ValidationError):
            self.assert_valid(input_schema, {**rpc_input, "interaction_value": 1.0})
        with self.assertRaises(jsonschema.ValidationError):
            self.assert_valid(input_schema, {**rpc_input, "event_type": "AI_ACTION", "source_type": "SYSTEM",
                                             "target_component_id": UUID, "position": None})

    def test_spatial_ui_capabilities_have_one_protocol_source(self) -> None:
        registry = yaml.safe_load((ROOT / "protocols/method-registry.yaml").read_text(encoding="utf-8"))["methods"]
        capabilities = {entry["method"]: entry["capability"] for entry in registry}
        self.assertEqual(method_capabilities(), capabilities)
        spatial_methods = {method: capability for method, capability in capabilities.items() if method.startswith("spatial_ui.")}
        self.assertTrue(spatial_methods)
        self.assertEqual(capabilities["system.health"], "system.health.read")
        self.assertEqual(spatial_methods["spatial_ui.state.load"], "spatial_ui.read")
        self.assertEqual(spatial_methods["spatial_ui.window.move"], "spatial_ui.input")
        self.assertEqual(spatial_methods["spatial_ui.target.lost"], "spatial_ui.system")
        self.assertEqual(spatial_methods["spatial_ui.component.create"], "spatial_ui.write")
        with tempfile.TemporaryDirectory() as directory:
            generated = Path(directory) / "MethodCapabilityTable.h"
            generated_python = Path(directory) / "generated_method_capabilities.py"
            subprocess.run(
                [
                    sys.executable,
                    str(ROOT / "scripts/generate_method_capabilities.py"),
                    "--output",
                    str(generated),
                    "--python-output",
                    str(generated_python),
                ],
                check=True,
                cwd=ROOT,
            )
            checked_in = ROOT / "libraries/astra-common/include/astra/common/MethodCapabilityTable.h"
            self.assertEqual(generated.read_text(encoding="utf-8"), checked_in.read_text(encoding="utf-8"))
            checked_in_python = ROOT / "scripts/generated_method_capabilities.py"
            self.assertEqual(
                generated_python.read_text(encoding="utf-8"),
                checked_in_python.read_text(encoding="utf-8"),
            )

    def test_component_query_carries_runtime_content_and_hierarchy(self) -> None:
        registry = yaml.safe_load((ROOT / "protocols/method-registry.yaml").read_text(encoding="utf-8"))["methods"]
        result_schema = next(entry["result_schema"] for entry in registry if entry["method"] == "spatial_ui.components")
        component = {
            "component_id": UUID, "component_type": "NOTIFICATION", "parent_id": None, "children": [],
            "bounds": {"x": 10.0, "y": 20.0, "width": 320.0, "height": 120.0}, "z_order": 10001,
            "transform": {"translation_x": 0.0, "translation_y": 0.0, "rotation_degrees": 0.0, "scale": 1.0},
            "opacity": 1.0, "visible": True, "enabled": True, "focusable": True, "interactive": True,
            "privacy_level": "PRIVATE_SCREEN_ONLY",
            "display_target": "PHONE", "accessibility_label": "Security confirmation", "lifecycle_state": "VISIBLE",
            "created_at": NOW, "updated_at": NOW,
            "content": {"title": "Security confirmation", "message": "Continue?", "severity": "CRITICAL",
                        "requires_action": True, "actions": ["Confirm"]},
        }
        self.assert_valid(result_schema, {"components": [component], "component_count": 1})
        with self.assertRaises(jsonschema.ValidationError):
            invalid = deepcopy(component)
            invalid["content"]["credential"] = "secret"
            self.assert_valid(result_schema, {"components": [invalid], "component_count": 1})

    def test_fixture_clients_load_without_third_party_runtime_parser(self) -> None:
        subprocess.run(
            [
                sys.executable,
                "-S",
                "-c",
                (
                    "import sys; "
                    f"sys.path.insert(0, {str(ROOT / 'scripts')!r}); "
                    "from p5_service_client import capability_for_method; "
                    "assert capability_for_method('system.health') == 'system.health.read'"
                ),
            ],
            check=True,
            cwd=ROOT,
        )

    def test_persisted_state_rejects_privacy_classification(self) -> None:
        state = self.fixtures()["protocols/spatial-ui/ui-state-v1.schema.json"]
        component = {
            "component_id": UUID, "component_type": "TASK_CARD",
            "bounds": {"x": 0, "y": 0, "width": 320, "height": 180},
            "z_order": 0, "visible": True, "focusable": True, "interactive": True,
            "display_target": "PHONE",
        }
        self.assert_valid("protocols/spatial-ui/ui-state-v1.schema.json", {**state, "components": [component]})
        with self.assertRaises(jsonschema.ValidationError):
            self.assert_valid(
                "protocols/spatial-ui/ui-state-v1.schema.json",
                {**state, "components": [{**component, "privacy_level": "PUBLIC"}]},
            )

    def test_method_results_enforce_uuid_ids_and_allow_empty_focus(self) -> None:
        status = {
            "status": "READY", "component_count": 0, "window_count": 0, "notification_count": 0,
            "focus_component_id": "", "projection_safe": True, "projection_frame_id": 0,
            "configuration_status": "VALID", "configuration_warning": "", "state_persistence_enabled": True,
            "reduce_motion": False, "high_contrast": False, "p4_release_status": MARKER,
        }
        focus = {"focus_component_id": UUID, "previous_focus_component_id": "", "reason": "keyboard navigation"}
        notification_created = {
            "notification_id": UUID, "notification_count": 1, "focus_component_id": UUID,
            "previous_focus_component_id": UUID_2, "focus_preempted": True,
            "reason": "critical notification", "component_id": UUID, "projection_layer_generated": True,
            "projection_frame_id": 1, "submitted_layer_count": 1,
        }
        notification_cleared = {
            "notification_id": UUID, "notification_count": 0, "focus_component_id": UUID_2,
            "previous_focus_component_id": UUID, "restored_focus_component_id": UUID_2,
            "focus_restored": True, "reason": "critical notification handled", "component_id": UUID,
            "projection_layer_generated": False, "projection_frame_id": 0, "submitted_layer_count": 0,
        }
        fixtures = {
            "schemas/protocols/methods/spatial-ui-status.result.schema.json": status,
            "schemas/protocols/methods/spatial-ui-focus.result.schema.json": focus,
            "schemas/protocols/methods/spatial-ui-notification-create.result.schema.json": notification_created,
            "schemas/protocols/methods/spatial-ui-notification-clear.result.schema.json": notification_cleared,
        }
        for relative, payload in fixtures.items():
            with self.subTest(schema=relative):
                self.assert_valid(relative, payload)
        for relative, payload, field in (
            ("schemas/protocols/methods/spatial-ui-status.result.schema.json", status, "focus_component_id"),
            ("schemas/protocols/methods/spatial-ui-focus.result.schema.json", focus, "previous_focus_component_id"),
            ("schemas/protocols/methods/spatial-ui-notification-create.result.schema.json", notification_created, "notification_id"),
            ("schemas/protocols/methods/spatial-ui-notification-clear.result.schema.json", notification_cleared, "restored_focus_component_id"),
        ):
            with self.subTest(schema=relative, field=field), self.assertRaises(jsonschema.ValidationError):
                self.assert_valid(relative, {**payload, field: "not-a-uuid"})

    def test_invalid_time_coordinate_and_size_are_rejected(self) -> None:
        fixtures = self.fixtures()
        component = fixtures["protocols/spatial-ui/spatial-component-v1.schema.json"]
        bad_bounds = {**component, "bounds": {"x": 0, "y": 0, "width": 0, "height": 10}}
        with self.assertRaises(jsonschema.ValidationError):
            self.assert_valid("protocols/spatial-ui/spatial-component-v1.schema.json", bad_bounds)
        event = fixtures["protocols/spatial-ui/spatial-input-event-v1.schema.json"]
        with self.assertRaises(jsonschema.ValidationError):
            self.assert_valid("protocols/spatial-ui/spatial-input-event-v1.schema.json", {**event, "timestamp": "not-a-time"})
        bad_position = {**event, "position": {"coordinate_system": "CAMERA", "x": 0, "y": 0}}
        with self.assertRaises(jsonschema.ValidationError):
            self.assert_valid("protocols/spatial-ui/spatial-input-event-v1.schema.json", bad_position)


if __name__ == "__main__":
    unittest.main()
