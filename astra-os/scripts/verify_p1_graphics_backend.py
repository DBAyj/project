#!/usr/bin/env python3
from __future__ import annotations

import sys
import re
from pathlib import Path


WINDOW_BACKEND = re.compile(
    r"graphics_api=(?P<graphics_api>\w+) renderer_interface=(?P<renderer_interface>\w+) "
    r"rhi_backend=(?P<rhi_backend>\w+) window_title=(?P<window_title>.+)$"
)
EXPECTED_WINDOWS = {"AstraOS Phone Display", "AstraOS Projection Display"}


def classify_log(text: str) -> str:
    reports = {
        match.group("window_title"): (match.group("graphics_api"), match.group("renderer_interface"), match.group("rhi_backend"))
        for line in text.splitlines()
        if (match := WINDOW_BACKEND.search(line))
    }
    if any(api != "Metal" or interface != "RHI" or backend != "Metal" for api, interface, backend in reports.values()):
        return "METAL_FALLBACK_DETECTED"
    if EXPECTED_WINDOWS.issubset(reports) and all(reports[window] == ("Metal", "RHI", "Metal") for window in EXPECTED_WINDOWS):
        return "METAL_CONFIRMED"
    return "METAL_REQUESTED_BUT_UNCONFIRMED"


def main() -> int:
    log = Path("runtime/logs/p1-shell.log")
    if not log.exists():
        print("METAL_INITIALIZATION_FAILED: log is missing")
        return 1
    text = log.read_text(encoding="utf-8", errors="replace")
    result = classify_log(text)
    print(result)
    if result == "METAL_CONFIRMED":
        return 0
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
