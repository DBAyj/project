from __future__ import annotations

from .candidate_merger import CandidateMergerConfig


class ConfidenceEvaluator:
    def __init__(self, config: CandidateMergerConfig) -> None:
        self._config = config

    def evaluate(
        self,
        confidence: float,
        *,
        missing_slot_count: int = 0,
        context_supports: bool = False,
        context_conflicts: bool = False,
    ) -> float:
        adjusted = confidence
        if missing_slot_count > 0:
            adjusted -= self._config.missing_slot_penalty
        if context_supports:
            adjusted += self._config.context_bonus
        if context_conflicts:
            adjusted -= self._config.conflict_penalty
        return max(0.0, min(1.0, adjusted))
