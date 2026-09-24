#!/usr/bin/env python3
"""Contract tests for the deterministic P4 fixture generator."""

from __future__ import annotations

import hashlib
import importlib.util
import json
import tempfile
import unittest
from pathlib import Path


SCRIPT = Path(__file__).with_name("generate.py")
SPEC = importlib.util.spec_from_file_location("p4_fixture_generator", SCRIPT)
MODULE = importlib.util.module_from_spec(SPEC)
assert SPEC and SPEC.loader
SPEC.loader.exec_module(MODULE)


class ProjectionFixtureGeneratorTests(unittest.TestCase):
    def test_generates_named_fixture_only_assets_with_stable_digest(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            destination = Path(temporary)
            MODULE.generate(destination)

            expected = {
                "checkerboard.png",
                "color-bars.png",
                "grayscale-ramp.png",
                "geometry-grid.png",
                "privacy-mask-test.png",
                "layer-composition-test.png",
                "homography-front.json",
                "homography-left-keystone.json",
                "homography-right-keystone.json",
                "homography-top-keystone.json",
                "homography-bottom-keystone.json",
                "invalid-homography.json",
            }
            self.assertEqual({path.name for path in destination.iterdir()}, expected)
            self.assertEqual(
                hashlib.sha256((destination / "checkerboard.png").read_bytes()).hexdigest(),
                "b9a3d7c5e3666262aebb03e14ab7cb76d967409c0925e100edfb0d0175bf6760",
            )
            profile = json.loads((destination / "homography-left-keystone.json").read_text(encoding="utf-8"))
            self.assertEqual(profile["fixture_id"], "left-keystone")
            self.assertEqual(profile["p3_integration_status"], "P3_REAL_SPATIAL_INTEGRATION_NOT_VERIFIED")


if __name__ == "__main__":
    unittest.main()
