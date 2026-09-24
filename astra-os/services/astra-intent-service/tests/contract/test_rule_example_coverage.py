from __future__ import annotations

from pathlib import Path
import unittest

import yaml


class RuleExampleCoverageTests(unittest.TestCase):
    def test_every_supported_core_intent_materializes_required_examples(self) -> None:
        root = Path(__file__).resolve().parents[4]
        document = yaml.safe_load((root / "config/intent-rules.yaml").read_text(encoding="utf-8"))
        generation = document["example_generation"]
        covered = 0
        for rule in document["rules"]:
            if not rule["enabled"] or rule["intent"] == "unknown":
                continue
            chinese = next(phrase for phrase in rule["exact_phrases"] if not phrase.isascii())
            english = next(phrase for phrase in rule["exact_phrases"] if phrase.isascii())
            examples = {
                "positive_zh": [template.format(phrase=chinese) for template in generation["positive_zh_templates"]],
                "positive_en": [template.format(phrase=english) for template in generation["positive_en_templates"]],
                "ambiguous": [template.format(phrase=chinese) for template in generation["ambiguous_templates"]],
                "negative": [template.format(phrase=chinese) for template in generation["negative_templates"]],
            }
            self.assertGreaterEqual(len(set(examples["positive_zh"])), 10, rule["intent"])
            self.assertGreaterEqual(len(set(examples["positive_en"])), 3, rule["intent"])
            self.assertGreaterEqual(len(set(examples["ambiguous"])), 3, rule["intent"])
            self.assertGreaterEqual(len(set(examples["negative"])), 3, rule["intent"])
            covered += 1
        self.assertEqual(covered, 29)


if __name__ == "__main__":
    unittest.main()
