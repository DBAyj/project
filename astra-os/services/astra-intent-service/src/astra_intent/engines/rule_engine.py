from __future__ import annotations

from dataclasses import dataclass
import json
from pathlib import Path
import re
from typing import Any

from jsonschema import Draft202012Validator
import yaml

from astra_intent.domain.errors import IntentServiceError
from astra_intent.domain.intent_candidate import CandidateSource, IntentCandidate


@dataclass(frozen=True, slots=True)
class RuleMatch:
    candidate: IntentCandidate
    priority: int
    match_type: str
    default_slots: dict[str, object]
    required_slots: tuple[str, ...]


@dataclass(frozen=True, slots=True)
class _Rule:
    rule_id: str
    intent: str
    priority: int
    locales: tuple[str, ...]
    confidence: float
    exact_phrases: tuple[str, ...]
    keywords: tuple[str, ...]
    templates: tuple[str, ...]
    synonyms: dict[str, tuple[str, ...]]
    negation_terms: tuple[str, ...]
    default_slots: dict[str, object]
    required_slots: tuple[str, ...]


class RuleEngine:
    def __init__(self, rules: tuple[_Rule, ...], *, version: str) -> None:
        self._rules = rules
        self.version = version

    @classmethod
    def from_files(cls, rules_path: Path, schema_path: Path) -> "RuleEngine":
        try:
            document = yaml.safe_load(rules_path.read_text(encoding="utf-8"))
            schema = json.loads(schema_path.read_text(encoding="utf-8"))
            errors = sorted(Draft202012Validator(schema).iter_errors(document), key=lambda error: list(error.path))
            if errors:
                path = ".".join(str(part) for part in errors[0].path) or "$"
                raise IntentServiceError(2010, f"Intent rule configuration is invalid at {path}: {errors[0].message}")
            rules = tuple(cls._parse_rule(value) for value in document["rules"] if value["enabled"])
            return cls(rules, version=document["version"])
        except IntentServiceError:
            raise
        except (OSError, ValueError, TypeError, yaml.YAMLError, json.JSONDecodeError) as error:
            raise IntentServiceError(2010, f"Intent rule configuration cannot be loaded: {error}") from error

    @staticmethod
    def _parse_rule(value: dict[str, Any]) -> _Rule:
        return _Rule(
            rule_id=value["id"],
            intent=value["intent"],
            priority=value["priority"],
            locales=tuple(value["locales"]),
            confidence=float(value["confidence"]),
            exact_phrases=tuple(phrase.casefold() for phrase in value["exact_phrases"]),
            keywords=tuple(keyword.casefold() for keyword in value.get("keywords", ())),
            templates=tuple(value.get("templates", ())),
            synonyms={key.casefold(): tuple(item.casefold() for item in values) for key, values in value.get("synonyms", {}).items()},
            negation_terms=tuple(term.casefold() for term in value.get("negation_terms", ())),
            default_slots=dict(value.get("default_slots", {})),
            required_slots=tuple(value.get("required_slots", ())),
        )

    def match(self, text: str, locale: str) -> tuple[RuleMatch, ...]:
        comparable = text.casefold()
        matches: list[RuleMatch] = []
        for rule in self._rules:
            if locale not in rule.locales or any(term in comparable for term in rule.negation_terms):
                continue
            match_type = self._match_type(rule, comparable)
            if match_type is None:
                continue
            confidence = max(0.0, rule.confidence - {"exact": 0.0, "template": 0.02, "keywords": 0.05}[match_type])
            candidate = IntentCandidate(
                rule.intent,
                confidence,
                CandidateSource.RULE_ENGINE,
                rule_id=rule.rule_id,
                evidence=(match_type,),
            )
            matches.append(
                RuleMatch(candidate, rule.priority, match_type, dict(rule.default_slots), rule.required_slots)
            )
        matches.sort(key=lambda item: (-item.priority, -item.candidate.confidence, item.candidate.intent))
        return tuple(matches)

    @classmethod
    def _match_type(cls, rule: _Rule, text: str) -> str | None:
        if text in rule.exact_phrases:
            return "exact"
        if any(cls._template_pattern(template).fullmatch(text) for template in rule.templates):
            return "template"
        if rule.keywords and all(cls._keyword_present(rule, keyword, text) for keyword in rule.keywords):
            return "keywords"
        return None

    @staticmethod
    def _template_pattern(template: str) -> re.Pattern[str]:
        escaped = re.escape(template.casefold())
        expression = re.sub(r"\\\{[^{}]+\\\}", r".+?", escaped)
        return re.compile(expression)

    @staticmethod
    def _keyword_present(rule: _Rule, keyword: str, text: str) -> bool:
        alternatives = (keyword, *rule.synonyms.get(keyword, ()))
        return any(alternative in text for alternative in alternatives)


class UnavailableRuleEngine(RuleEngine):
    def __init__(self) -> None:
        super().__init__((), version="unavailable")

    def match(self, text: str, locale: str) -> tuple[RuleMatch, ...]:
        del text, locale
        raise IntentServiceError(2301, "Rule engine is unavailable")
