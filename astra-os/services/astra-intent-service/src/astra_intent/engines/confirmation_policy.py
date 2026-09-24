from __future__ import annotations

from dataclasses import dataclass

from astra_intent.domain.intent import IntentContext
from astra_intent.domain.intent_result import ExecutionPolicy


@dataclass(frozen=True, slots=True)
class ConfirmationPolicyConfig:
    auto_execute_threshold: float = 0.85
    confirmation_threshold: float = 0.65
    reject_threshold: float = 0.45


@dataclass(frozen=True, slots=True)
class PolicyDecision:
    execution_policy: ExecutionPolicy
    error_code: int | None
    reason: str
    confirmation_message: str | None = None


class ConfirmationPolicy:
    _privacy_targets = {
        "set_privacy_public": "PUBLIC",
        "set_privacy_room_only": "ROOM_ONLY",
        "set_privacy_authorized_person": "AUTHORIZED_PERSON",
        "set_privacy_private_screen_only": "PRIVATE_SCREEN_ONLY",
        "set_privacy_no_projection": "NO_PROJECTION",
    }

    def __init__(self, config: ConfirmationPolicyConfig) -> None:
        self._config = config

    def decide(
        self,
        intent: str,
        confidence: float,
        context: IntentContext,
        *,
        ambiguous: bool = False,
    ) -> PolicyDecision:
        if ambiguous:
            return PolicyDecision(ExecutionPolicy.ASK_CLARIFICATION, 2104, "ambiguous-intent")
        if intent == "unknown":
            return PolicyDecision(ExecutionPolicy.REJECT, 2001, "unknown-intent")
        if confidence < self._config.reject_threshold:
            return PolicyDecision(ExecutionPolicy.REJECT, 2007, "confidence-below-reject-threshold")
        if confidence < self._config.confirmation_threshold:
            return PolicyDecision(ExecutionPolicy.ASK_CLARIFICATION, 2104, "confidence-requires-clarification")
        risk_message = self._risk_message(intent, context)
        if risk_message is not None or confidence < self._config.auto_execute_threshold:
            message = risk_message or "该投影操作置信度不足以自动执行，是否继续？"
            return PolicyDecision(ExecutionPolicy.REQUIRE_CONFIRMATION, 2101, "confirmation-required", message)
        return PolicyDecision(ExecutionPolicy.AUTO_EXECUTE, None, "high-confidence-low-risk")

    def _risk_message(self, intent: str, context: IntentContext) -> str | None:
        if intent == "stop_projection" and context.projection_state in {"ACTIVE", "PAUSED"}:
            return "该操作会停止正在进行的展示，是否继续？"
        if intent == "hide_projection_content":
            return "该操作会清除当前投影场景，是否继续？"
        if intent == "set_privacy_authorized_person":
            return "该操作会切换为授权人员可见，是否继续？"
        target = self._privacy_targets.get(intent)
        if context.current_privacy_level in {"PRIVATE_SCREEN_ONLY", "NO_PROJECTION"} and target in {
            "PUBLIC",
            "ROOM_ONLY",
            "AUTHORIZED_PERSON",
        }:
            return "该操作会扩大内容的外部可见范围，是否继续？"
        return None
