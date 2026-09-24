#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
from pathlib import Path


MARKER = "P4_RELEASE_BASELINE_FINAL"
COMPONENT_IDS = [
    "7abf7b1e-dc4f-4b7c-96cc-c3dd628e1ca9",
    "c38c0e21-96f1-4975-8cf3-fbfaa58c7cda",
    "6f24950b-299d-4c80-9f82-08e0eb8e3327",
]


def component(index: int, privacy: str = "PUBLIC") -> dict[str, object]:
    return {
        "component_id": COMPONENT_IDS[index],
        "component_type": "TASK_CARD" if index else "SPATIAL_WINDOW",
        "bounds": {"x": 40 + index * 180, "y": 80 + index * 40, "width": 320, "height": 180},
        "privacy_level": privacy,
        "visible": True,
        "interactive": True,
        "accessibility_label": f"Fixture component {index + 1}",
    }


def fixtures() -> dict[str, dict[str, object]]:
    base = {"schema_version": "1.0", "p4_release_status": MARKER}
    return {
        "single-window-layout.json": {**base, "layout_mode": "FREEFORM", "components": [component(0)]},
        "multi-window-layout.json": {**base, "layout_mode": "GRID", "components": [component(0), component(1), component(2)]},
        "stacked-task-cards.json": {**base, "layout_mode": "STACK", "components": [component(1), component(2)]},
        "radial-actions.json": {**base, "layout_mode": "RADIAL", "components": [component(0), component(1), component(2)]},
        "notification-stack.json": {**base, "layout_mode": "STACK", "notifications": [{"severity": "WARNING", "timeout_ms": 3000}, {"severity": "CRITICAL", "timeout_ms": 0}]},
        "privacy-mixed-components.json": {**base, "layout_mode": "GRID", "components": [component(0, "PUBLIC"), component(1, "PRIVATE_SCREEN_ONLY"), component(2, "NO_PROJECTION")]},
        "invalid-overlap-layout.json": {**base, "expected_error_code": 5302, "layout_mode": "FREEFORM", "components": [component(0), {**component(1), "bounds": component(0)["bounds"]}]},
        "invalid-outside-layout.json": {**base, "expected_error_code": 5204, "layout_mode": "FREEFORM", "components": [{**component(0), "bounds": {"x": 5000, "y": 5000, "width": 320, "height": 180}}]},
        "keyboard-focus-flow.json": {**base, "events": [{"type": "KEY_PRESS", "key": "TAB", "expected_target": COMPONENT_IDS[1]}, {"type": "KEY_PRESS", "key": "TAB", "expected_target": COMPONENT_IDS[2]}]},
        "mouse-interaction-flow.json": {**base, "events": [{"type": "POINTER_PRESS", "x": 120, "y": 140}, {"type": "POINTER_MOVE", "x": 260, "y": 200}, {"type": "POINTER_RELEASE", "x": 260, "y": 200}]},
        "simulated-gesture-flow.json": {**base, "events": [{"type": "GESTURE_SELECT", "component_id": COMPONENT_IDS[0]}, {"type": "GESTURE_SCALE", "scale": 1.2}, {"type": "GESTURE_RELEASE"}]},
    }


def generate(output: Path) -> None:
    output.mkdir(parents=True, exist_ok=True)
    for name, payload in fixtures().items():
        (output / name).write_text(json.dumps(payload, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    generate(args.output)
    print(f"generated={len(fixtures())} output={args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
