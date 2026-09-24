from __future__ import annotations

from pathlib import Path
import unittest

from astra_intent.adapters.deterministic_model import DeterministicIntentModel
from astra_intent.adapters.model_adapter import IntentModelAdapter
from astra_intent.domain.intent import IntentContext
from astra_intent.domain.intent_candidate import CandidateSource


class DeterministicIntentModelTests(unittest.IsolatedAsyncioTestCase):
    def setUp(self) -> None:
        root = Path(__file__).resolve().parents[4]
        self.model = DeterministicIntentModel.from_rules_file(root / "config/intent-rules.yaml")
        self.context = IntentContext("IDLE", "development_workspace", "PUBLIC", "demo-device")

    async def test_satisfies_adapter_and_is_deterministic(self) -> None:
        self.assertIsInstance(self.model, IntentModelAdapter)

        first = await self.model.predict("显示设备三维图", "zh-CN", self.context)
        second = await self.model.predict("显示设备三维图", "zh-CN", self.context)

        self.assertEqual(first, second)
        self.assertEqual(first[0].intent, "project_3d_model")
        self.assertEqual(first[0].source, CandidateSource.LOCAL_MODEL)

    async def test_recognizes_english_variant_and_returns_ranked_candidates(self) -> None:
        candidates = await self.model.predict("display the device model", "en-US", self.context)

        self.assertGreaterEqual(len(candidates), 1)
        self.assertEqual(candidates[0].intent, "project_3d_model")
        self.assertGreater(candidates[0].confidence, 0.5)
        self.assertEqual(list(candidates), sorted(candidates, key=lambda item: (-item.confidence, item.intent)))

    async def test_unknown_or_unsupported_locale_returns_no_candidate(self) -> None:
        self.assertEqual(await self.model.predict("帮我购买机票", "zh-CN", self.context), ())
        self.assertEqual(await self.model.predict("project the device model", "fr-FR", self.context), ())


if __name__ == "__main__":
    unittest.main()
