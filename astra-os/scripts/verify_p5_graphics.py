#!/usr/bin/env python3
from __future__ import annotations

import json
import re
import subprocess
import sys
import time
from pathlib import Path

from PIL import Image, ImageStat


ROOT = Path(__file__).resolve().parents[1]
MARKER = "P4_RELEASE_BASELINE_FINAL"
EXPECTED = {"AstraOS Phone Display": "p5-phone.png", "AstraOS Projection Display": "p5-projection.png"}
BACKEND = re.compile(r"graphics_api=(\w+) renderer_interface=(\w+) rhi_backend=(\w+) window_title=(.+)$")
FIRST_FRAME = re.compile(r"p5_ui_first_frame=true elapsed_ms=(\d+) window_visible=true workspace_visible=true window_title=(.+)$")


def screenshot_metrics(path: Path) -> dict[str, object]:
    image = Image.open(path).convert("RGB")
    content = image.crop((0, min(100, image.height // 10), image.width, image.height))
    variance = sum(ImageStat.Stat(content).var) / 3.0
    colors = content.resize((128, 128)).getcolors(128 * 128) or []
    if image.width < 700 or image.height < 700 or variance < 100.0 or len(colors) < 128:
        raise RuntimeError(
            f"blank or undersized screenshot {path.name}: size={image.size}, variance={variance}, colors={len(colors)}"
        )
    return {"width": image.width, "height": image.height, "variance": variance, "sampled_colors": len(colors)}


def main() -> int:
    log_path = ROOT / "runtime/logs/p5-shell.log"
    if not log_path.is_file():
        print("FAIL: P5 Shell log is missing")
        return 1
    log_text = log_path.read_text(encoding="utf-8", errors="replace")
    project_qml_warnings = [
        line for line in log_text.splitlines() if "qrc:/qt/qml/Astra/Shell/" in line and line.startswith("qt.qml")
    ]
    if project_qml_warnings:
        print("FAIL: project QML warnings detected\n" + "\n".join(project_qml_warnings))
        return 1
    backends = {match.group(4): match.groups()[:3] for line in log_text.splitlines() if (match := BACKEND.search(line))}
    if any(backends.get(title) != ("Metal", "RHI", "Metal") for title in EXPECTED):
        print(f"FAIL: actual Metal/RHI evidence is incomplete: {backends}")
        return 1
    frames = {match.group(2): int(match.group(1)) for line in log_text.splitlines() if (match := FIRST_FRAME.search(line))}
    if any(title not in frames or frames[title] > 1200 for title in EXPECTED):
        print(f"FAIL: P5 UI first-frame evidence is incomplete or slow: {frames}")
        return 1
    if "p4_projection_output_first_frame=true window_visible=true" not in log_text:
        print("FAIL: P4 projection first frame is missing")
        return 1

    window_data = json.loads(
        subprocess.check_output(["swift", str(ROOT / "scripts/list_p5_windows.swift")], text=True)
    )
    windows = {entry["title"]: entry for entry in window_data}
    if set(windows) != set(EXPECTED):
        print(f"FAIL: expected two visible AstraOS windows, got {sorted(windows)}")
        return 1
    output = ROOT / "docs/reports/evidence"
    output.mkdir(parents=True, exist_ok=True)
    screenshots: dict[str, dict[str, object]] = {}
    for title, filename in EXPECTED.items():
        destination = output / filename
        subprocess.run(["screencapture", "-x", "-l", str(windows[title]["window_id"]), str(destination)], check=True)
        screenshots[title] = screenshot_metrics(destination)

    fullscreen_markers = ("p5_fullscreen_entered=true", "p5_fullscreen_exited=true")
    for _ in range(100):
        log_text = log_path.read_text(encoding="utf-8", errors="replace")
        if all(marker in log_text for marker in fullscreen_markers):
            break
        time.sleep(0.1)
    else:
        print("FAIL: live fullscreen enter/exit cycle evidence is missing")
        return 1

    sys.path.insert(0, str(ROOT / "scripts"))
    from p5_service_client import FIXTURE_WINDOW_ID, invoke

    socket_path = str(ROOT / "runtime/spatial-ui/sockets/astra-spatial-ui.sock")
    token = (ROOT / "runtime/spatial-ui/credentials/client.token").read_text(encoding="utf-8").strip()
    p4_socket_path = str(ROOT / "runtime/state/p5-projection.sock")
    p4_token = (ROOT / "runtime/spatial-ui/credentials/projection.token").read_text(encoding="utf-8").strip()
    projected_frame = invoke(
        p4_socket_path,
        "projection.output.frame",
        {"output_id": "p4-window-projection"},
        p4_token,
        capability="projection.output.read",
    )
    public_labels = projected_frame.get("public_labels", [])
    if "P5 PUBLIC fixture task" not in public_labels or any("PRIVATE" in label for label in public_labels):
        print(f"FAIL: P4 readable public TaskSurface evidence is invalid: {public_labels}")
        return 1
    move = invoke(socket_path, "spatial_ui.window.move", {"window_id": FIXTURE_WINDOW_ID, "x": 140, "y": 96}, token)
    resize = invoke(
        socket_path,
        "spatial_ui.window.resize",
        {"window_id": FIXTURE_WINDOW_ID, "width": 480, "height": 280},
        token,
    )
    saved = invoke(socket_path, "spatial_ui.state.save", {}, token)
    invoke(socket_path, "spatial_ui.reset", {}, token)
    loaded = invoke(socket_path, "spatial_ui.state.load", {}, token)
    status = invoke(socket_path, "spatial_ui.status", {}, token)
    if (
        not move.get("window_id")
        or not resize.get("window_id")
        or not saved.get("state_path")
        or loaded.get("selected_tab") != "tasks"
    ):
        print("FAIL: live move/resize/state persistence interaction failed")
        return 1
    if (
        status.get("p4_release_status") != MARKER
        or status.get("component_count", 0) < 2
        or status.get("window_count", 0) < 1
        or not status.get("focus_component_id")
        or not status.get("projection_safe")
        or status.get("projection_frame_id") != 0
    ):
        print(f"FAIL: live P5 status is incomplete: {status}")
        return 1

    report = ROOT / "docs/reports/p5-graphics-report.md"
    report.write_text(
        "# P5 Graphics Evidence\n\n"
        f"Baseline: `{MARKER}`\n\n"
        "Result: `P5_GRAPHICS_PASSED`\n\n"
        "Both real Qt windows rendered through Metal/RHI, exposed visible spatial workspaces, and produced nonblank captured pixels. "
        "The Projection window also retained the P4 fixture first frame.\n\n"
        f"- Phone first frame: {frames['AstraOS Phone Display']} ms\n"
        f"- Projection first frame: {frames['AstraOS Projection Display']} ms\n"
        f"- Phone screenshot: {screenshots['AstraOS Phone Display']}\n"
        f"- Projection screenshot: {screenshots['AstraOS Projection Display']}\n"
        "- Metal backend: `Metal` through Qt `RHI` for both windows\n"
        "- P4 projection first frame: visible\n"
        f"- P4 readable public labels: {public_labels}\n"
        "- Private labels in P4 output: none\n"
        "- Restored state privacy: policy reclassified, projection remained safely cleared\n"
        "- Fullscreen enter/exit cycle: passed\n"
        "- Project QML warnings: 0\n\n"
        "Evidence: [Phone screenshot](evidence/p5-phone.png), [Projection screenshot](evidence/p5-projection.png).\n",
        encoding="utf-8",
    )
    interaction = ROOT / "docs/reports/p5-interaction-report.md"
    interaction.write_text(
        "# P5 Interaction Evidence\n\n"
        f"Baseline: `{MARKER}`\n\n"
        "Result: `P5_INTERACTION_PASSED`\n\n"
        "The live service accepted focus, window open/move/resize, state save/load, and maintained one valid focus owner. "
        "Restored components were reclassified through policy and did not reactivate public projection. "
        "The real Shell completed a fullscreen enter/exit cycle while both spatial workspaces remained renderable. "
        "P2 and P3 inputs remain deterministic fixture adapters.\n",
        encoding="utf-8",
    )
    print("P5_GRAPHICS_PASSED")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
