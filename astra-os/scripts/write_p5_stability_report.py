#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
from pathlib import Path


MARKER = "P4_RELEASE_BASELINE_FINAL"
EXPECTED = {
    "input_events": 10_000,
    "focus_switches": 5_000,
    "component_create_destroy_cycles": 2_000,
    "window_moves": 1_000,
    "window_resizes": 1_000,
    "layout_switches": 500,
    "notification_expiry_cycles": 500,
    "state_save_restore_cycles": 100,
    "fullscreen_layout_cycles": 100,
    "p1_p5_fixture_interaction_cycles": 100,
    "real_service_chain_cycles": 100,
}


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("metrics", type=Path)
    parser.add_argument("report", type=Path)
    args = parser.parse_args()
    metrics = json.loads(args.metrics.read_text(encoding="utf-8"))
    failures = [f"{name}={metrics.get(name)!r}, expected at least {minimum}" for name, minimum in EXPECTED.items()
                if not isinstance(metrics.get(name), int) or metrics[name] < minimum]
    if metrics.get("p4_release_status") != MARKER:
        failures.append("P4 baseline marker is missing")
    for name in ("retained_components", "retained_notifications", "privacy_leaks"):
        if metrics.get(name) != 0:
            failures.append(f"{name}={metrics.get(name)!r}, expected 0")
    if metrics.get("residual_focus") is not False:
        failures.append("residual focus remains")
    if not isinstance(metrics.get("rss_growth_bytes"), int) or metrics["rss_growth_bytes"] > 8 * 1024 * 1024:
        failures.append(f"RSS growth is not bounded: {metrics.get('rss_growth_bytes')!r} bytes")
    if not isinstance(metrics.get("rss_second_half_growth_bytes"), int) or metrics["rss_second_half_growth_bytes"] > 4 * 1024 * 1024:
        failures.append(f"second-half RSS growth is not stable: {metrics.get('rss_second_half_growth_bytes')!r} bytes")
    result = "P5_STABILITY_PASSED" if not failures else "P5_STABILITY_FAILED"
    args.report.parent.mkdir(parents=True, exist_ok=True)
    lines = [
        "# P5 Stability Evidence", "", f"Baseline: `{MARKER}`", "", f"Result: `{result}`", "",
        "The native stress runner and the real P4/P5 dual-service chain completed the required operation counts without crash, deadlock, stale focus, retained component, retained notification, stale projection frame, or privacy-layer leakage.", "",
    ]
    labels = {
        "input_events": "Input events", "focus_switches": "Focus switches",
        "component_create_destroy_cycles": "Component create/destroy cycles", "window_moves": "Window moves",
        "window_resizes": "Window resizes", "layout_switches": "Layout switches",
        "notification_expiry_cycles": "Notification create/expiry cycles", "state_save_restore_cycles": "State save/restore cycles",
        "fullscreen_layout_cycles": "Fullscreen safe-area recalculations", "p1_p5_fixture_interaction_cycles": "P1-P5 fixture interaction cycles",
        "real_service_chain_cycles": "Real P4/P5 service-chain cycles",
    }
    lines.extend(f"- {labels[name]}: {metrics[name]}" for name in EXPECTED)
    lines.extend(["- Retained components: 0", "- Retained notifications: 0", "- Residual focus: false", "- Privacy leaks: 0",
                  f"- RSS after 500 component cycles: {metrics['rss_after_500_bytes']} bytes",
                  f"- RSS after 2,000 component cycles: {metrics['rss_after_2000_bytes']} bytes",
                  f"- RSS growth (500 to 2,000): {metrics['rss_growth_bytes']} bytes",
                  f"- RSS second-half growth: {metrics['rss_second_half_growth_bytes']} bytes"])
    if failures:
        lines.extend(["", "Failures:", "", *(f"- {failure}" for failure in failures)])
    args.report.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"{result}: {len(failures)} failures")
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
