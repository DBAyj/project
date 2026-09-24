from __future__ import annotations

from dataclasses import dataclass
from enum import StrEnum
from typing import Any


class CandidateSource(StrEnum):
    RULE_ENGINE = "RULE_ENGINE"
    LOCAL_MODEL = "LOCAL_MODEL"
    FALLBACK = "FALLBACK"


@dataclass(frozen=True, slots=True)
class IntentCandidate:
    intent: str
    confidence: float
    source: CandidateSource
    rule_id: str | None = None
    evidence: tuple[str, ...] = ()

    def to_dict(self, *, include_schema: bool = False) -> dict[str, Any]:
        value: dict[str, Any] = {
            "intent": self.intent,
            "confidence": round(max(0.0, min(1.0, self.confidence)), 6),
            "source": self.source.value,
        }
        if self.rule_id is not None:
            value["rule_id"] = self.rule_id
        if self.evidence:
            value["evidence"] = list(self.evidence)
        if include_schema:
            value = {"schema_version": "1.0", **value}
        return value
