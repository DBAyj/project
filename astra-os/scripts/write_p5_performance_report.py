#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
from pathlib import Path


MARKER = "P4_RELEASE_BASELINE_FINAL"
LIMITS = {
    "layout_p95_ms": 8.0,
    "input_p95_ms": 4.0,
    "focus_p95_ms": 3.0,
    "state_update_p95_ms": 5.0,
    "projection_layer_p95_ms": 5.0,
    "framework_first_frame_ms": 1200.0,
}


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("metrics", type=Path)
    parser.add_argument("report", type=Path)
    args = parser.parse_args()
    metrics = json.loads(args.metrics.read_text(encoding="utf-8"))
    failures = [f"{name}={metrics.get(name)!r} exceeds {limit}" for name, limit in LIMITS.items()
                if not isinstance(metrics.get(name), (int, float)) or metrics[name] > limit]
    if metrics.get("stable_fps_floor", 0) < 30.0:
        failures.append(f"stable_fps_floor={metrics.get('stable_fps_floor')!r} is below 30")
    if metrics.get("p4_release_status") != MARKER:
        failures.append("P4 baseline marker is missing")
    result = "P5_PERFORMANCE_PASSED" if not failures else "P5_PERFORMANCE_FAILED"
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(
        "# P5 Performance Evidence\n\n"
        f"Baseline: `{MARKER}`\n\n"
        f"Result: `{result}`\n\n"
        "Measured on the local Apple Silicon development host with 100 visible fixture components. "
        "The first-frame value in this report measures the framework layout and P4 layer batch; the actual Metal UI first frame is recorded by the graphics gate.\n\n"
        f"- Layout P95: {metrics['layout_p95_ms']:.6f} ms ({metrics['layout_iterations']} iterations)\n"
        f"- Input routing P95: {metrics['input_p95_ms']:.6f} ms ({metrics['input_iterations']} iterations)\n"
        f"- Focus switching P95: {metrics['focus_p95_ms']:.6f} ms ({metrics['focus_iterations']} iterations)\n"
        f"- State update P95: {metrics['state_update_p95_ms']:.6f} ms\n"
        f"- ProjectionLayer batch P95: {metrics['projection_layer_p95_ms']:.6f} ms ({metrics['layer_iterations']} iterations)\n"
        f"- Framework first frame: {metrics['framework_first_frame_ms']:.6f} ms\n"
        f"- Mean framework FPS: {metrics['mean_fps']:.3f}\n"
        f"- Stable framework FPS floor (frame P95): {metrics['stable_fps_floor']:.3f}\n"
        + ("\nFailures:\n\n" + "\n".join(f"- {failure}" for failure in failures) + "\n" if failures else ""),
        encoding="utf-8",
    )
    print(f"{result}: {len(failures)} threshold failures")
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
