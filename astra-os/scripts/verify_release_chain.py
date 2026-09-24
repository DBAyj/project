#!/usr/bin/env python3
"""Verify the formal AstraOS predecessor release-tag chain."""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
RELEASES = (
    ("astra-os-p1-v0.1.0-alpha.1", "0.1.0-alpha.1"),
    ("astra-os-p2-v0.2.0-alpha.1", "0.2.0-alpha.1"),
    ("astra-os-p3-v0.3.0-alpha.1", "0.3.0-alpha.1"),
    ("astra-os-p4-v0.4.0-alpha.1", "0.4.0-alpha.1"),
)


def git(*args: str, check: bool = True) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        ["git", *args], cwd=ROOT, check=check, text=True,
        stdout=subprocess.PIPE, stderr=subprocess.PIPE,
    )


def main() -> int:
    failures: list[str] = []
    for tag, expected_version in RELEASES:
        if git("rev-parse", "--verify", "--quiet", f"refs/tags/{tag}", check=False).returncode != 0:
            failures.append(f"missing release tag: {tag}")
            continue
        version = git("show", f"{tag}:VERSION").stdout.strip()
        if version != expected_version:
            failures.append(f"{tag} VERSION is {version!r}, expected {expected_version!r}")

    for (earlier, _), (later, _) in zip(RELEASES, RELEASES[1:]):
        if git("merge-base", "--is-ancestor", earlier, later, check=False).returncode != 0:
            failures.append(f"release ancestry is not continuous: {earlier} -> {later}")
    final_tag = RELEASES[-1][0]
    if git("rev-parse", "--verify", "--quiet", f"refs/tags/{final_tag}", check=False).returncode == 0:
        if git("merge-base", "--is-ancestor", final_tag, "HEAD", check=False).returncode != 0:
            failures.append(f"current HEAD does not descend from {final_tag}")

    root_version = (ROOT / "VERSION").read_text(encoding="utf-8").strip()
    if root_version != "0.5.0-alpha.1":
        failures.append(f"root VERSION is {root_version!r}, expected '0.5.0-alpha.1'")

    if failures:
        for failure in failures:
            print(f"FAIL: {failure}", file=sys.stderr)
        return 1
    print("P1_P4_RELEASE_CHAIN_VERIFIED")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
