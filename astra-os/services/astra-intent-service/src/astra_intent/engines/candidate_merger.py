from __future__ import annotations

from dataclasses import dataclass

from astra_intent.domain.intent_candidate import CandidateSource, IntentCandidate


@dataclass(frozen=True, slots=True)
class CandidateMergerConfig:
    rule_engine_weight: float = 0.60
    local_model_weight: float = 0.40
    agreement_bonus: float = 0.08
    context_bonus: float = 0.05
    conflict_penalty: float = 0.20
    missing_slot_penalty: float = 0.25


class CandidateMerger:
    def __init__(self, config: CandidateMergerConfig) -> None:
        self._config = config

    def merge(
        self,
        rule_candidates: tuple[IntentCandidate, ...],
        model_candidates: tuple[IntentCandidate, ...],
    ) -> tuple[IntentCandidate, ...]:
        best_rules = self._best_by_intent(rule_candidates)
        best_models = self._best_by_intent(model_candidates)
        conflict = bool(rule_candidates and model_candidates and rule_candidates[0].intent != model_candidates[0].intent)
        merged: list[IntentCandidate] = []
        for intent in best_rules.keys() | best_models.keys():
            rule = best_rules.get(intent)
            model = best_models.get(intent)
            evidence = self._evidence(rule, model)
            if rule is not None and model is not None:
                confidence = (
                    self._config.rule_engine_weight * rule.confidence
                    + self._config.local_model_weight * model.confidence
                    + self._config.agreement_bonus
                )
                source = CandidateSource.RULE_ENGINE
                evidence.add("engine-agreement")
            else:
                candidate = rule or model
                assert candidate is not None
                confidence = candidate.confidence
                source = candidate.source
            if conflict:
                confidence -= self._config.conflict_penalty
                evidence.add("engine-conflict")
            merged.append(
                IntentCandidate(
                    intent,
                    max(0.0, min(1.0, confidence)),
                    source,
                    rule_id=rule.rule_id if rule is not None else None,
                    evidence=tuple(sorted(evidence)),
                )
            )
        merged.sort(key=lambda candidate: (-candidate.confidence, candidate.intent))
        return tuple(merged)

    @staticmethod
    def _best_by_intent(candidates: tuple[IntentCandidate, ...]) -> dict[str, IntentCandidate]:
        result: dict[str, IntentCandidate] = {}
        for candidate in candidates:
            current = result.get(candidate.intent)
            if current is None or candidate.confidence > current.confidence:
                result[candidate.intent] = candidate
        return result

    @staticmethod
    def _evidence(rule: IntentCandidate | None, model: IntentCandidate | None) -> set[str]:
        evidence: set[str] = set()
        if rule is not None:
            evidence.update(rule.evidence)
        if model is not None:
            evidence.update(model.evidence)
        return evidence
