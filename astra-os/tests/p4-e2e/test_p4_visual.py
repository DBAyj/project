#!/usr/bin/env python3
from __future__ import annotations

import hashlib
import importlib.util
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "tools" / "projection-fixture-generator" / "generate.py"
SPEC = importlib.util.spec_from_file_location("p4_fixture_generator", SCRIPT)
MODULE = importlib.util.module_from_spec(SPEC)
assert SPEC and SPEC.loader
SPEC.loader.exec_module(MODULE)


class VisualFixtureTests(unittest.TestCase):
    def test_known_fixture_images_have_independent_baseline_digests(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            output = Path(temporary)
            MODULE.generate(output)
            expected = {
                "checkerboard.png": "b9a3d7c5e3666262aebb03e14ab7cb76d967409c0925e100edfb0d0175bf6760",
                "color-bars.png": "46245ea7fac7ffa4a4b3653ec40faf598465911ef66b003c59590759a321c5a8",
                "privacy-mask-test.png": "09232ec061fe283706d398ac30fbb6049360aed57e92e977433e946d56f93dab",
            }
            for name, digest in expected.items():
                with self.subTest(image=name):
                    self.assertEqual(hashlib.sha256((output / name).read_bytes()).hexdigest(), digest)

    def test_homography_profiles_stay_fixture_only(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            output = Path(temporary)
            MODULE.generate(output)
            profiles = list(output.glob("homography-*.json"))
            self.assertEqual(len(profiles), 5)
            for profile in profiles:
                self.assertIn('"P3_REAL_SPATIAL_INTEGRATION_NOT_VERIFIED"', profile.read_text(encoding="utf-8"))


if __name__ == "__main__":
    unittest.main()
