from __future__ import annotations

from collections import defaultdict
from difflib import SequenceMatcher
from pathlib import Path
import re
import unicodedata

import yaml

from astra_intent.domain.errors import IntentServiceError
from astra_intent.domain.intent import IntentContext
from astra_intent.domain.intent_candidate import CandidateSource, IntentCandidate
from astra_intent.extraction.text_normalizer import TextNormalizer


class DeterministicIntentModel:
    """Small, repeatable classifier backed by versioned local examples."""

    def __init__(self, examples: dict[str, dict[str, tuple[str, ...]]], *, minimum_score: float = 0.45) -> None:
        self._examples = examples
        self._minimum_score = minimum_score
        self._normalizer = TextNormalizer()

    @classmethod
    def from_rules_file(cls, rules_path: Path) -> "DeterministicIntentModel":
        try:
            document = yaml.safe_load(rules_path.read_text(encoding="utf-8"))
            generation = document["example_generation"]
            examples: dict[str, dict[str, list[str]]] = defaultdict(lambda: defaultdict(list))
            for rule in document["rules"]:
                if not rule["enabled"] or rule["intent"] == "unknown":
                    continue
                for phrase in rule["exact_phrases"]:
                    locale = "en-US" if phrase.isascii() else "zh-CN"
                    if locale in rule["locales"]:
                        examples[locale][rule["intent"]].append(phrase)
                chinese = next((phrase for phrase in rule["exact_phrases"] if not phrase.isascii()), None)
                english = next((phrase for phrase in rule["exact_phrases"] if phrase.isascii()), None)
                if chinese is not None and "zh-CN" in rule["locales"]:
                    examples["zh-CN"][rule["intent"]].extend(
                        template.format(phrase=chinese) for template in generation["positive_zh_templates"]
                    )
                if english is not None and "en-US" in rule["locales"]:
                    examples["en-US"][rule["intent"]].extend(
                        template.format(phrase=english) for template in generation["positive_en_templates"]
                    )
            frozen = {
                locale: {intent: tuple(values) for intent, values in intents.items()}
                for locale, intents in examples.items()
            }
            return cls(frozen)
        except (OSError, TypeError, KeyError, yaml.YAMLError) as error:
            raise IntentServiceError(2302, f"Deterministic model examples cannot be loaded: {error}") from error

    async def predict(
        self,
        text: str,
        locale: str,
        context: IntentContext,
    ) -> tuple[IntentCandidate, ...]:
        del context
        if locale not in self._examples:
            return ()
        normalized = self._normalizer.normalize(text)
        candidates: list[IntentCandidate] = []
        for intent, phrases in self._examples[locale].items():
            score = max(self._similarity(normalized, self._normalizer.normalize(phrase)) for phrase in phrases)
            if score >= self._minimum_score:
                candidates.append(
                    IntentCandidate(
                        intent,
                        min(0.95, score * 0.95),
                        CandidateSource.LOCAL_MODEL,
                        evidence=("deterministic-example-similarity",),
                    )
                )
        candidates.sort(key=lambda item: (-item.confidence, item.intent))
        return tuple(candidates[:3])

    @classmethod
    def _similarity(cls, left: str, right: str) -> float:
        sequence = SequenceMatcher(a=left, b=right, autojunk=False).ratio()
        left_units = cls._units(left)
        right_units = cls._units(right)
        union = left_units | right_units
        jaccard = len(left_units & right_units) / len(union) if union else 0.0
        return max(sequence, jaccard)

    @staticmethod
    def _units(text: str) -> set[str]:
        words = re.findall(r"[a-z0-9]+", text.casefold())
        characters = [character for character in text if not character.isspace() and not unicodedata.category(character).startswith("P")]
        units = set(words)
        units.update("".join(characters[index : index + 2]) for index in range(max(0, len(characters) - 1)))
        return units


class UnavailableIntentModel:
    async def predict(self, text: str, locale: str, context: IntentContext) -> tuple[IntentCandidate, ...]:
        del text, locale, context
        raise IntentServiceError(2302, "Local model adapter is unavailable")
