#!/usr/bin/env python3
from __future__ import annotations

import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def require(condition: bool, message: str, failures: list[str]) -> None:
    print(("PASS: " if condition else "FAIL: ") + message)
    if not condition:
        failures.append(message)


def evidence(path: Path) -> dict[str, object]:
    return json.loads(path.read_text(encoding="utf-8")) if path.is_file() else {}


def main() -> int:
    failures: list[str] = []
    spatial_config = (ROOT / "config/spatial.yaml").read_text(encoding="utf-8")
    require("version: 0.3.0-alpha.1" in spatial_config, "P3 service version is 0.3.0-alpha.1", failures)
    for relative in ("services/astra-spatial-service/src/main.cpp", "apps/astra-shell/src/clients/SpatialServiceClient.cpp", "tests/p3-e2e/test_p3_live_service.py", "assets/spatial-fixtures/desk-front.png"):
        require((ROOT / relative).is_file(), f"P3 artifact exists: {relative}", failures)
    performance = evidence(ROOT / "runtime/tmp/p3-performance.json")
    stability = evidence(ROOT / "runtime/tmp/p3-stability.json")
    live = evidence(ROOT / "runtime/tmp/p3-live-acceptance.json")
    camera = evidence(ROOT / "runtime/tmp/p3-camera-evidence.json")
    require(performance.get("status") == "PASSED" and float(performance.get("pipeline_p95_ms", 999)) <= 70 and float(performance.get("fps", 0)) >= 15, "image-pipeline performance evidence", failures)
    require(stability.get("status") == "PASSED" and stability.get("frame_cycles") == 10000 and stability.get("selection_cycles") == 1000 and stability.get("homography_cycles") == 500 and stability.get("source_switch_cycles") == 100 and stability.get("service_start_stop_cycles") == 100 and stability.get("resource_residue") is False, "P3 stability evidence", failures)
    require(live.get("status") == "PASSED" and live.get("final_state") == "LOST" and live.get("safe_pause_code") == 3305, "actual run/stop spatial safety evidence", failures)
    require(camera.get("status") in {"PASSED", "SKIPPED_ENVIRONMENT_LIMITATION"}
            and isinstance(camera.get("camera_enumerated"), bool)
            and (camera.get("status") == "PASSED" or bool(camera.get("limitation"))), "camera acceptance or measured environment limitation", failures)
    status = "PASSED" if not failures else "FAILED"
    report = ROOT / "docs/reports/p3-verification-report.md"
    report.write_text("# P3 Verification Report\n\n" + f"- Status: `{status}`\n" + "- P1/P2 regression: executed by `make p3-verify`\n" + f"- Pipeline P95: {performance.get('pipeline_p95_ms', 'missing')} ms\n" + f"- Throughput: {performance.get('fps', 'missing')} FPS\n" + f"- Event latency P95: {performance.get('event_latency_p95_ms', 'missing')} ms\n" + f"- Stability frames: {stability.get('frame_cycles', 'missing')}\n" + f"- Camera evidence: {camera.get('status', 'missing')}\n" + f"- Live final state: {live.get('final_state', 'missing')}\n" + ("\n## Failures\n\n" + "\n".join(f"- {item}" for item in failures) + "\n" if failures else "\n## Result\n\nP3 verification evidence is complete.\n"), encoding="utf-8")
    return 0 if not failures else 1


if __name__ == "__main__":
    raise SystemExit(main())
