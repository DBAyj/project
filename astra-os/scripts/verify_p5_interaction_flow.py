#!/usr/bin/env python3
from __future__ import annotations

import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def events(name: str) -> list[dict[str, object]]:
    return json.loads((ROOT / "assets/spatial-ui-fixtures" / name).read_text(encoding="utf-8"))["events"]


def main() -> int:
    expected = {
        "mouse-interaction-flow.json": ["POINTER_PRESS", "POINTER_MOVE", "POINTER_RELEASE"],
        "keyboard-focus-flow.json": ["KEY_PRESS", "KEY_PRESS"],
        "simulated-gesture-flow.json": ["GESTURE_SELECT", "GESTURE_SCALE", "GESTURE_RELEASE"],
    }
    for name, sequence in expected.items():
        actual = [event["type"] for event in events(name)]
        if actual != sequence:
            print(f"FAIL: {name} expected {sequence}, got {actual}")
            return 1
    print("PASS: deterministic mouse, keyboard, and simulated gesture fixture flows verified")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
