from __future__ import annotations

import importlib.util
import hashlib
from pathlib import Path
import struct
import tempfile
import unittest


MODULE_PATH = Path(__file__).resolve().parents[1] / "generate_fixtures.py"
EXPECTED = {
    "desk-front.png",
    "desk-oblique.png",
    "wall-front.png",
    "wall-oblique.png",
    "multiple-surfaces.png",
    "low-light.png",
    "partial-occlusion.png",
    "no-surface.png",
    "invalid-small-surface.png",
    "calibration-grid.png",
}
EXPECTED_SHA256 = {
    "calibration-grid.png": "8aff3a9552a490fc033e24c323262771967013ca84520bd1a8cbb7925b649383",
    "desk-front.png": "f02f812fcb5c3b628eaa3e3b8e0881a7d8e81835027e3562982224902c303c11",
    "desk-oblique.png": "eebbbcfedc3b698890b39d462179198675f2c1c04e19cc2d1d86fef75d6f858d",
    "invalid-small-surface.png": "e6992d0485ee6bfe3ccaadf146fac78bbfe50807939e1e452925619124e001fa",
    "low-light.png": "c507c5fdfff67f401dbb2f41d57cff992f6c93afd6082d31c6d587d93c795023",
    "multiple-surfaces.png": "6ee18be4ffc7263cf5bfedf612d0732f04de594698553bd0e9eaab777e70795b",
    "no-surface.png": "d16f0aaf80c4e270ac9245e343ebc574c1eeebd0f095c129faa9e37bb88a80ee",
    "partial-occlusion.png": "7976790c59f65952e846f092c932dfa03dbad04c0f4e2d423fcf3bf047918a9b",
    "wall-front.png": "d9b574d0cdedd9d80e8cbbb5b77e86bbfaa5fcf938b74f12c29dde3fa26f007c",
    "wall-oblique.png": "cfba5975d9fc3a688faaea0c851916c0a9218ec6e8f6c4010bd392b1e0b8ba7d",
}


def load_generator():
    spec = importlib.util.spec_from_file_location("spatial_fixture_generator", MODULE_PATH)
    if spec is None or spec.loader is None:
        raise RuntimeError("fixture generator module cannot be loaded")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class SpatialFixtureGeneratorTests(unittest.TestCase):
    def test_generates_declared_png_fixtures_deterministically(self) -> None:
        generator = load_generator()
        with tempfile.TemporaryDirectory() as first_dir, tempfile.TemporaryDirectory() as second_dir:
            first = Path(first_dir)
            second = Path(second_dir)
            generator.generate(first)
            generator.generate(second)
            self.assertEqual({path.name for path in first.glob("*.png")}, EXPECTED)
            for name in EXPECTED:
                first_bytes = (first / name).read_bytes()
                second_bytes = (second / name).read_bytes()
                self.assertEqual(first_bytes, second_bytes, name)
                self.assertTrue(first_bytes.startswith(b"\x89PNG\r\n\x1a\n"), name)
                width, height = struct.unpack(">II", first_bytes[16:24])
                self.assertEqual((width, height), (1280, 720), name)
                self.assertGreater(len(first_bytes), 1000, name)
                self.assertEqual(hashlib.sha256(first_bytes).hexdigest(), EXPECTED_SHA256[name], name)

    def test_surface_and_empty_fixtures_are_visibly_distinct(self) -> None:
        generator = load_generator()
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory)
            generator.generate(output)
            self.assertNotEqual((output / "desk-front.png").read_bytes(), (output / "no-surface.png").read_bytes())
            self.assertNotEqual((output / "desk-oblique.png").read_bytes(), (output / "wall-oblique.png").read_bytes())
            self.assertNotEqual((output / "multiple-surfaces.png").read_bytes(), (output / "partial-occlusion.png").read_bytes())


if __name__ == "__main__":
    unittest.main()
