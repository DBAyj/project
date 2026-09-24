#!/usr/bin/env python3
from __future__ import annotations

import argparse
from pathlib import Path


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--log", type=Path, default=Path("runtime/logs/p5-qml-test.log"))
    args = parser.parse_args()
    if not args.log.is_file():
        print(f"FAIL: QML test log is missing: {args.log}")
        return 1
    project_markers = ("apps/astra-shell/qml/", "qrc:/qt/qml/Astra/Shell/")
    warning_markers = ("Unable to assign", "TypeError", "ReferenceError", "binding loop", "is not a type", "module \"")
    findings = [line for line in args.log.read_text(encoding="utf-8", errors="replace").splitlines()
                if any(project in line for project in project_markers) and any(warning in line for warning in warning_markers)]
    if findings:
        print("FAIL: P5 project QML warnings detected")
        print("\n".join(findings))
        return 1
    print("PASS: P5 project QML warning count = 0")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
