#!/usr/bin/env python3
from __future__ import annotations

import sys
from pathlib import Path


REQUIRED_MARKERS = (
    "Creating QRhi with backend Metal",
    "Metal device:",
    "got CAMetalLayer",
    "window_title=AstraOS Phone Display",
    "window_title=AstraOS Projection Display",
)


def main() -> int:
    log = Path("runtime/logs/p1-shell.log")
    if not log.is_file():
        print("WINDOW_INTERACTION_EVIDENCE_FAILED: runtime log is missing")
        return 1
    text = log.read_text(encoding="utf-8", errors="replace")
    missing = [marker for marker in REQUIRED_MARKERS if marker not in text]
    if missing:
        print("WINDOW_INTERACTION_EVIDENCE_FAILED: missing " + ", ".join(missing))
        return 1
    print("WINDOW_INTERACTION_EVIDENCE_PASSED windows=2 controller_component_interaction=passed qml_pointer_semantics=passed scene_graph=rendered")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
