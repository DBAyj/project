#!/usr/bin/env python3
from __future__ import annotations

import sys
from pathlib import Path


def main() -> int:
    path = Path("runtime/logs/p1-shell.log")
    if not path.exists():
        print("FAIL: P1 QML log is missing")
        return 1
    project_markers = ("qrc:/qt/qml/Astra/Shell/", "apps/astra-shell/qml/")
    warning_markers = ("QML ", "TypeError", "Unable to assign", "binding loop", "module \"")
    findings = [line for line in path.read_text(encoding="utf-8", errors="replace").splitlines()
                if any(marker in line for marker in project_markers) and any(marker in line for marker in warning_markers)]
    if findings:
        print("FAIL: project QML warnings detected")
        print("\n".join(findings))
        return 1
    print("PASS: project QML warning count = 0")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
