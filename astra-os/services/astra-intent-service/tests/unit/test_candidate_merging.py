from __future__ import annotations

import unittest

from astra_intent.domain.intent_candidate import CandidateSource, IntentCandidate
from astra_intent.engines.candidate_merger import CandidateMerger, CandidateMergerConfig
from astra_intent.engines.confidence_evaluator import ConfidenceEvaluator


class CandidateMergerTests(unittest.TestCase):
    def setUp(self) -> None:
        self.config = CandidateMergerConfig(
            rule_engine_weight=0.60,
            local_model_weight=0.40,
            agreement_bonus=0.08,
            context_bonus=0.05,
            conflict_penalty=0.20,
            missing_slot_penalty=0.25,
        )
        self.merger = CandidateMerger(self.config)

    def test_agreement_combines_sources_and_adds_bonus(self) -> None:
        rule = IntentCandidate("project_3d_model", 0.90, CandidateSource.RULE_ENGINE)
        model = IntentCandidate("project_3d_model", 0.80, CandidateSource.LOCAL_MODEL)

        merged = self.merger.merge((rule,), (model,))

        self.assertEqual(len(merged), 1)
        self.assertEqual(merged[0].intent, "project_3d_model")
        self.assertAlmostEqual(merged[0].confidence, 0.94)
        self.assertIn("engine-agreement", merged[0].evidence)

    def test_conflicting_top_candidates_are_penalized_and_ranked(self) -> None:
        rule = IntentCandidate("start_projection", 0.95, CandidateSource.RULE_ENGINE)
        model = IntentCandidate("stop_projection", 0.90, CandidateSource.LOCAL_MODEL)

        merged = self.merger.merge((rule,), (model,))

        self.assertEqual([candidate.intent for candidate in merged], ["start_projection", "stop_projection"])
        self.assertAlmostEqual(merged[0].confidence, 0.75)
        self.assertAlmostEqual(merged[1].confidence, 0.70)
        self.assertTrue(all("engine-conflict" in candidate.evidence for candidate in merged))

    def test_single_engine_candidate_preserves_observed_confidence(self) -> None:
        rule = IntentCandidate("show_system_status", 0.98, CandidateSource.RULE_ENGINE)

        self.assertEqual(self.merger.merge((rule,), ())[0].confidence, 0.98)


class ConfidenceEvaluatorTests(unittest.TestCase):
    def test_missing_slots_and_context_apply_configured_adjustments(self) -> None:
        config = CandidateMergerConfig(missing_slot_penalty=0.25, context_bonus=0.05, conflict_penalty=0.20)
        evaluator = ConfidenceEvaluator(config)

        self.assertAlmostEqual(evaluator.evaluate(0.90, missing_slot_count=1), 0.65)
        self.assertAlmostEqual(evaluator.evaluate(0.90, context_supports=True), 0.95)
        self.assertAlmostEqual(evaluator.evaluate(0.90, context_conflicts=True), 0.70)


if __name__ == "__main__":
    unittest.main()
