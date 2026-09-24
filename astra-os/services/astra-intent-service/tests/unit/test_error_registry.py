from __future__ import annotations

from pathlib import Path
import unittest

from astra_intent.infrastructure.error_registry import ErrorRegistry


class ErrorRegistryTests(unittest.TestCase):
    def setUp(self) -> None:
        root = Path(__file__).resolve().parents[4]
        self.registry = ErrorRegistry.from_file(root / "protocols/error-codes.yaml")

    def test_all_p2_codes_are_unique_complete_and_owned(self) -> None:
        expected = set(range(2001, 2011)) | set(range(2101, 2105)) | set(range(2201, 2205)) | set(range(2301, 2305))
        descriptors = self.registry.all()

        self.assertTrue(expected.issubset({descriptor.code for descriptor in descriptors}))
        self.assertEqual(len({descriptor.code for descriptor in descriptors}), len(descriptors))
        self.assertEqual(len({descriptor.symbol for descriptor in descriptors}), len(descriptors))
        self.assertTrue(all(descriptor.module and descriptor.message_en and descriptor.message_zh for descriptor in descriptors))

    def test_lookup_and_unknown_fallback_are_stable(self) -> None:
        self.assertEqual(self.registry.lookup(2201).symbol, "PROMPT_INJECTION_RISK_DETECTED")
        self.assertTrue(self.registry.lookup(2201).audit_required)
        self.assertEqual(self.registry.lookup(9998).code, 9001)


if __name__ == "__main__":
    unittest.main()
