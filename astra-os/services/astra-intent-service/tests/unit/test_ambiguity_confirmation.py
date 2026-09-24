from __future__ import annotations

import unittest

from astra_intent.domain.intent import IntentContext, IntentSlots
from astra_intent.domain.intent_candidate import CandidateSource, IntentCandidate
from astra_intent.domain.intent_result import ExecutionPolicy
from astra_intent.engines.ambiguity_detector import AmbiguityDetector
from astra_intent.engines.confirmation_policy import ConfirmationPolicy, ConfirmationPolicyConfig


class AmbiguityDetectorTests(unittest.TestCase):
    def setUp(self) -> None:
        self.detector = AmbiguityDetector(delta=0.10)

    def test_close_candidates_are_ambiguous(self) -> None:
        candidates = (
            IntentCandidate("start_projection", 0.82, CandidateSource.RULE_ENGINE),
            IntentCandidate("stop_projection", 0.75, CandidateSource.LOCAL_MODEL),
        )

        result = self.detector.detect("投影", candidates, IntentSlots())

        self.assertTrue(result.ambiguous)
        self.assertEqual(result.error_code, 2006)
        self.assertEqual(result.clarification.options, ("启动投影", "停止投影"))

    def test_mutually_exclusive_text_and_privacy_conflicts_are_ambiguous(self) -> None:
        start_stop = self.detector.detect("打开然后关闭投影", (), IntentSlots())
        privacy = self.detector.detect(
            "给大家看，只在手机上显示", (), IntentSlots(), slot_conflicts=("PUBLIC", "PRIVATE_SCREEN_ONLY")
        )

        self.assertTrue(start_stop.ambiguous)
        self.assertTrue(privacy.ambiguous)
        self.assertIn("隐私", privacy.clarification.question)

    def test_missing_required_slots_asks_for_specific_information(self) -> None:
        result = self.detector.detect(
            "把那个放到那里",
            (IntentCandidate("project_3d_model", 0.7, CandidateSource.LOCAL_MODEL),),
            IntentSlots(),
            required_slots=("model_id", "target_space"),
        )

        self.assertTrue(result.ambiguous)
        self.assertEqual(result.error_code, 2104)
        self.assertIn("模型", result.clarification.question)
        self.assertIn("位置", result.clarification.question)


class ConfirmationPolicyTests(unittest.TestCase):
    def setUp(self) -> None:
        self.policy = ConfirmationPolicy(
            ConfirmationPolicyConfig(auto_execute_threshold=0.85, confirmation_threshold=0.65, reject_threshold=0.45)
        )
        self.idle_public = IntentContext("IDLE", "development_workspace", "PUBLIC", "demo-device")

    def test_high_confidence_low_risk_intent_auto_executes(self) -> None:
        decision = self.policy.decide("project_3d_model", 0.95, self.idle_public)

        self.assertEqual(decision.execution_policy, ExecutionPolicy.AUTO_EXECUTE)
        self.assertIsNone(decision.error_code)

    def test_risky_actions_require_confirmation_even_at_high_confidence(self) -> None:
        active = IntentContext("ACTIVE", "development_workspace", "PUBLIC", "demo-device")
        private = IntentContext("IDLE", "development_workspace", "PRIVATE_SCREEN_ONLY", "demo-device")

        self.assertEqual(
            self.policy.decide("stop_projection", 1.0, active).execution_policy,
            ExecutionPolicy.REQUIRE_CONFIRMATION,
        )
        self.assertEqual(
            self.policy.decide("set_privacy_public", 0.99, private).execution_policy,
            ExecutionPolicy.REQUIRE_CONFIRMATION,
        )

    def test_confidence_thresholds_reject_clarify_or_confirm(self) -> None:
        self.assertEqual(self.policy.decide("project_3d_model", 0.40, self.idle_public).execution_policy, ExecutionPolicy.REJECT)
        self.assertEqual(self.policy.decide("project_3d_model", 0.55, self.idle_public).execution_policy, ExecutionPolicy.ASK_CLARIFICATION)
        self.assertEqual(
            self.policy.decide("project_3d_model", 0.75, self.idle_public).execution_policy,
            ExecutionPolicy.REQUIRE_CONFIRMATION,
        )

    def test_ambiguity_always_prevents_execution(self) -> None:
        decision = self.policy.decide("start_projection", 0.99, self.idle_public, ambiguous=True)

        self.assertEqual(decision.execution_policy, ExecutionPolicy.ASK_CLARIFICATION)
        self.assertEqual(decision.error_code, 2104)


if __name__ == "__main__":
    unittest.main()
