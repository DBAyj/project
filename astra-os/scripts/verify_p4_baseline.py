#!/usr/bin/env python3
from __future__ import annotations

import json
import sys
from pathlib import Path


def main() -> int:
    if len(sys.argv) != 3:
        print("usage: verify_p4_baseline.py METRICS_JSON REPORT_MD", file=sys.stderr)
        return 2
    metrics = json.loads(Path(sys.argv[1]).read_text(encoding="utf-8"))
    limits = {"p95_ms": 500.0, "first_frame_ms": 1000.0, "safe_clear_ms": 1000.0}
    failures = [name for name, limit in limits.items() if metrics.get(name, float("inf")) > limit]
    report = Path(sys.argv[2])
    report.parent.mkdir(parents=True, exist_ok=True)
    report.write_text(
        "# P4 Performance Evidence\n\n"
        "Fixture-only runtime marker: `P3_REAL_SPATIAL_INTEGRATION_NOT_VERIFIED`.\n\n"
        f"- Frames: {metrics.get('frames')}\n"
        f"- P95 full pipeline: {metrics.get('p95_ms'):.3f} ms\n"
        f"- Mean throughput: {metrics.get('fps'):.3f} FPS\n"
        f"- First frame: {metrics.get('first_frame_ms'):.3f} ms\n"
        f"- Safe clear: {metrics.get('safe_clear_ms'):.3f} ms\n"
        f"- Result: {'P4_PERFORMANCE_PASSED' if not failures else 'P4_PERFORMANCE_FAILED: ' + ', '.join(failures)}\n",
        encoding="utf-8",
    )
    if failures:
        print("P4_PERFORMANCE_FAILED: " + ", ".join(failures))
        return 1
    print("P4_PERFORMANCE_PASSED")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
