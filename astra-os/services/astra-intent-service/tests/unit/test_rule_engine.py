from __future__ import annotations

from pathlib import Path
import tempfile
import unittest

import yaml

from astra_intent.domain.errors import IntentServiceError
from astra_intent.engines.rule_engine import RuleEngine


class RuleEngineTests(unittest.TestCase):
    def setUp(self) -> None:
        self._temporary = tempfile.TemporaryDirectory()
        root = Path(self._temporary.name)
        self.rules_path = root / "rules.yaml"
        self.schema_path = root / "schema.json"
        repository_root = Path(__file__).resolve().parents[4]
        self.schema_path.write_text(
            (repository_root / "schemas/intent-rules.schema.json").read_text(encoding="utf-8"),
            encoding="utf-8",
        )

    def tearDown(self) -> None:
        self._temporary.cleanup()

    def _engine(self, rules: list[dict[str, object]]) -> RuleEngine:
        self.rules_path.write_text(
            yaml.safe_dump({"version": "1.0", "example_generation": self._generation(), "rules": rules}, allow_unicode=True),
            encoding="utf-8",
        )
        return RuleEngine.from_files(self.rules_path, self.schema_path)

    @staticmethod
    def _generation() -> dict[str, list[str]]:
        return {
            "positive_zh_templates": [f"前缀{index}{{phrase}}" for index in range(10)],
            "positive_en_templates": [f"prefix{index} {{phrase}}" for index in range(3)],
            "ambiguous_templates": [f"maybe{index} {{phrase}}" for index in range(3)],
            "negative_templates": [f"not{index} {{phrase}}" for index in range(3)],
        }

    @staticmethod
    def _rule(**overrides: object) -> dict[str, object]:
        value: dict[str, object] = {
            "id": "stop",
            "intent": "stop_projection",
            "priority": 100,
            "locales": ["zh-CN"],
            "enabled": True,
            "confidence": 1.0,
            "exact_phrases": ["停止投影"],
        }
        value.update(overrides)
        return value

    def test_exact_phrase_returns_rule_candidate_and_defaults(self) -> None:
        engine = self._engine([self._rule(default_slots={"display_target": "projection_screen"})])

        matches = engine.match("停止投影", "zh-CN")

        self.assertEqual(matches[0].candidate.intent, "stop_projection")
        self.assertEqual(matches[0].candidate.confidence, 1.0)
        self.assertEqual(matches[0].match_type, "exact")
        self.assertEqual(matches[0].default_slots, {"display_target": "projection_screen"})

    def test_template_matches_variable_text(self) -> None:
        engine = self._engine(
            [
                self._rule(
                    id="project-template",
                    intent="project_3d_model",
                    confidence=0.98,
                    exact_phrases=[],
                    templates=["把{model}投到{target}"],
                    required_slots=["model_id", "target_space"],
                )
            ]
        )

        match = engine.match("把设备模型投到墙上", "zh-CN")[0]

        self.assertEqual(match.candidate.intent, "project_3d_model")
        self.assertEqual(match.match_type, "template")
        self.assertEqual(match.required_slots, ("model_id", "target_space"))

    def test_keyword_synonyms_match_without_rewriting_input(self) -> None:
        engine = self._engine(
            [
                self._rule(
                    id="project-keywords",
                    intent="project_3d_model",
                    confidence=0.94,
                    exact_phrases=[],
                    keywords=["展示", "设备"],
                    synonyms={"展示": ["显示"], "设备": ["模型"]},
                )
            ]
        )

        match = engine.match("请显示模型", "zh-CN")[0]

        self.assertEqual(match.candidate.intent, "project_3d_model")
        self.assertEqual(match.match_type, "keywords")

    def test_negation_prevents_positive_rule_match(self) -> None:
        engine = self._engine(
            [
                self._rule(
                    id="project-keywords",
                    intent="project_3d_model",
                    exact_phrases=[],
                    keywords=["投影", "模型"],
                    negation_terms=["不要", "别"],
                )
            ]
        )

        self.assertEqual(engine.match("不要投影模型", "zh-CN"), ())

    def test_priority_orders_competing_matches_and_locale_filters(self) -> None:
        engine = self._engine(
            [
                self._rule(id="low", intent="start_projection", priority=10, exact_phrases=["投影"]),
                self._rule(id="high", intent="project_3d_model", priority=200, exact_phrases=["投影"]),
            ]
        )

        matches = engine.match("投影", "zh-CN")

        self.assertEqual([item.candidate.intent for item in matches], ["project_3d_model", "start_projection"])
        self.assertEqual(engine.match("投影", "en-US"), ())

    def test_invalid_configuration_is_rejected(self) -> None:
        self.rules_path.write_text(
            yaml.safe_dump({"version": "1.0", "example_generation": self._generation(), "rules": [{"unexpected": True}]}),
            encoding="utf-8",
        )

        with self.assertRaises(IntentServiceError) as caught:
            RuleEngine.from_files(self.rules_path, self.schema_path)

        self.assertEqual(caught.exception.code, 2010)


if __name__ == "__main__":
    unittest.main()
