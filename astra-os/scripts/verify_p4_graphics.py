#!/usr/bin/env python3
from __future__ import annotations

import re
import sys
from pathlib import Path


WINDOW_BACKEND = re.compile(
    r"graphics_api=(?P<graphics_api>\w+) renderer_interface=(?P<renderer_interface>\w+) "
    r"rhi_backend=(?P<rhi_backend>\w+) window_title=(?P<window_title>.+)$"
)
OUTPUT_FRAME = re.compile(
    r"p4_projection_output_first_frame=true window_visible=true window_title=AstraOS Projection Display "
    r"p3_integration_status=P3_REAL_SPATIAL_INTEGRATION_NOT_VERIFIED$"
)
EXPECTED_WINDOWS = {"AstraOS Phone Display", "AstraOS Projection Display"}


def classify_log(text: str) -> str:
    reports = {
        match.group("window_title"): (match.group("graphics_api"), match.group("renderer_interface"), match.group("rhi_backend"))
        for line in text.splitlines()
        if (match := WINDOW_BACKEND.search(line))
    }
    if any(api != "Metal" or interface != "RHI" or backend != "Metal" for api, interface, backend in reports.values()):
        return "P4_METAL_FALLBACK_DETECTED"
    if EXPECTED_WINDOWS.issubset(reports) and all(reports[window] == ("Metal", "RHI", "Metal") for window in EXPECTED_WINDOWS) \
            and any(OUTPUT_FRAME.search(line) for line in text.splitlines()):
        return "P4_METAL_CONFIRMED"
    return "P4_METAL_REQUESTED_BUT_UNCONFIRMED"


def main() -> int:
    log = Path("runtime/logs/p4-shell.log")
    if not log.exists():
        print("P4_METAL_INITIALIZATION_FAILED: log is missing")
        return 1
    result = classify_log(log.read_text(encoding="utf-8", errors="replace"))
    print(result)
    return 0 if result == "P4_METAL_CONFIRMED" else 1


if __name__ == "__main__":
    raise SystemExit(main())
