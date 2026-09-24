from __future__ import annotations

from dataclasses import dataclass
from enum import StrEnum
from typing import Any

from .confirmation import ConfirmationRequest
from .errors import IntentError
from .intent import IntentSlots
from .intent_candidate import IntentCandidate


class ExecutionPolicy(StrEnum):
    AUTO_EXECUTE = "AUTO_EXECUTE"
    REQUIRE_CONFIRMATION = "REQUIRE_CONFIRMATION"
    ASK_CLARIFICATION = "ASK_CLARIFICATION"
    REJECT = "REJECT"


@dataclass(frozen=True, slots=True)
class ProcessingEvidence:
    rule_engine_used: bool = False
    local_model_used: bool = False
    fallback_used: bool = False
    cache_hit: bool = False
    duration_ms: float = 0.0

    def to_dict(self) -> dict[str, object]:
        return {
            "rule_engine_used": self.rule_engine_used,
            "local_model_used": self.local_model_used,
            "fallback_used": self.fallback_used,
            "cache_hit": self.cache_hit,
            "duration_ms": round(max(0.0, self.duration_ms), 3),
        }


@dataclass(frozen=True, slots=True)
class Clarification:
    question: str
    options: tuple[str, ...]

    def to_dict(self) -> dict[str, object]:
        return {"question": self.question, "options": list(self.options)}


@dataclass(frozen=True, slots=True)
class IntentResult:
    request_id: str
    trace_id: str
    intent: str
    confidence: float
    execution_policy: ExecutionPolicy
    candidates: tuple[IntentCandidate, ...]
    slots: IntentSlots
    normalized_text: str
    processing: ProcessingEvidence
    created_at: str
    ambiguous: bool = False
    warnings: tuple[str, ...] = ()
    error: IntentError | None = None
    clarification: Clarification | None = None
    confirmation: ConfirmationRequest | None = None

    @property
    def requires_confirmation(self) -> bool:
        return self.execution_policy is ExecutionPolicy.REQUIRE_CONFIRMATION

    def to_dict(self) -> dict[str, Any]:
        confirmation = None
        if self.confirmation is not None:
            confirmation = {
                "confirmation_id": self.confirmation.confirmation_id,
                "message": self.confirmation.message,
                "expires_at": self.confirmation.expires_at,
            }
        return {
            "schema_version": "2.0",
            "request_id": self.request_id,
            "trace_id": self.trace_id,
            "intent": self.intent,
            "confidence": round(max(0.0, min(1.0, self.confidence)), 6),
            "execution_policy": self.execution_policy.value,
            "requires_confirmation": self.requires_confirmation,
            "ambiguous": self.ambiguous,
            "candidates": [candidate.to_dict() for candidate in self.candidates],
            "slots": self.slots.to_dict(),
            "normalized_text": self.normalized_text,
            "processing": self.processing.to_dict(),
            "warnings": list(self.warnings),
            "error": None if self.error is None else self.error.to_dict(),
            "clarification": None if self.clarification is None else self.clarification.to_dict(),
            "confirmation": confirmation,
            "created_at": self.created_at,
        }
