#!/usr/bin/env python3
"""Check stable whitespace formatting for every tracked P1 source file."""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SUFFIXES = {".cpp", ".h", ".qml", ".py", ".sh"}
NAMES = {"CMakeLists.txt", "Makefile"}


def tracked_source_files() -> list[Path]:
    output = subprocess.check_output(["git", "ls-files"], cwd=ROOT, text=True)
    paths = [Path(line) for line in output.splitlines()]
    return [path for path in paths if path.suffix in SUFFIXES or path.name in NAMES]


def check_file(path: Path) -> list[str]:
    data = (ROOT / path).read_bytes()
    findings: list[str] = []
    if not data.endswith(b"\n"):
        findings.append(f"{path}: missing trailing newline")
    if b"\r\n" in data:
        findings.append(f"{path}: CRLF line ending")
    for line_number, line in enumerate(data.splitlines(), start=1):
        if line.rstrip(b" \t") != line:
            findings.append(f"{path}:{line_number}: trailing whitespace")
    return findings


def main() -> int:
    findings = [finding for path in tracked_source_files() for finding in check_file(path)]
    if findings:
        print("FORMAT_CHECK_FAILED")
        print("\n".join(findings))
        return 1
    print("FORMAT_CHECK_PASSED tracked_source_files=" + str(len(tracked_source_files())))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
