#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def main() -> int:
    checks = {
        "apps/astra-shell/qml/components/AstraSpatialButton.qml": ("Accessible.name", "activeFocusOnTab", "Accessible.Button"),
        "apps/astra-shell/qml/components/AstraSpatialSlider.qml": ("Accessible.name", "activeFocusOnTab", "Accessible.Slider"),
        "apps/astra-shell/qml/components/AstraSpatialToggle.qml": ("Accessible.name", "activeFocusOnTab", "Accessible.CheckBox"),
        "apps/astra-shell/qml/spatial/SpatialWorkspace.qml": ("Accessible.name", "Accessible.description", "Accessible.Pane"),
        "apps/astra-shell/qml/spatial/SpatialTaskCard.qml": ("Accessible.name", "Accessible.description", "Accessible.Grouping"),
        "apps/astra-shell/qml/spatial/SpatialNotificationView.qml": ("Accessible.name", "Accessible.description", "Accessible.AlertMessage"),
        "apps/astra-shell/qml/spatial/SpatialPrivacyBadge.qml": ("Accessible.name", "受限内容", "Accessible.StaticText"),
    }
    failures: list[str] = []
    for relative, markers in checks.items():
        content = (ROOT / relative).read_text(encoding="utf-8")
        missing = [marker for marker in markers if marker not in content]
        if missing:
            failures.append(f"{relative}: missing {', '.join(missing)}")
    test = (ROOT / "apps/astra-shell/tests/qml/tst_p5_spatial_ui.qml").read_text(encoding="utf-8")
    for marker in ("activeFocusOnTab", "Accessible.name", "test_spatial_controls_are_keyboard_focusable_and_named",
                   "test_accessibility_preferences_change_runtime_theme", "animationDuration", "workspaceFocusOutline"):
        if marker not in test:
            failures.append(f"P5 QML accessibility test missing {marker}")
    if failures:
        print("\n".join(f"FAIL: {failure}" for failure in failures))
        return 1
    print(f"PASS: P5 accessibility semantics verified across {len(checks)} core QML controls/views")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
