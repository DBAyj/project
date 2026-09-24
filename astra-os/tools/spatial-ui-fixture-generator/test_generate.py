from __future__ import annotations

import json
from pathlib import Path
import tempfile
import unittest

from generate import MARKER, fixtures, generate


class SpatialUIFixtureGeneratorTests(unittest.TestCase):
    def test_generates_complete_deterministic_fixture_set(self) -> None:
        with tempfile.TemporaryDirectory() as first, tempfile.TemporaryDirectory() as second:
            generate(Path(first))
            generate(Path(second))
            self.assertEqual(11, len(list(Path(first).glob("*.json"))))
            self.assertEqual(sorted(path.name for path in Path(first).glob("*.json")), sorted(fixtures()))
            for name in fixtures():
                self.assertEqual((Path(first) / name).read_bytes(), (Path(second) / name).read_bytes())
                self.assertEqual(MARKER, json.loads((Path(first) / name).read_text(encoding="utf-8"))["p4_release_status"])


if __name__ == "__main__":
    unittest.main()
