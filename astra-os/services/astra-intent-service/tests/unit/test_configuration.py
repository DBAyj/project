from __future__ import annotations

from pathlib import Path
import tempfile
import unittest

from astra_intent.domain.errors import IntentServiceError
from astra_intent.infrastructure.configuration import IntentConfiguration


class IntentConfigurationTests(unittest.TestCase):
    def test_repository_configuration_loads_with_monotonic_thresholds(self) -> None:
        root = Path(__file__).resolve().parents[4]

        configuration = IntentConfiguration.load(root)

        self.assertEqual(configuration.service["version"], "0.2.0-alpha.1")
        self.assertLessEqual(configuration.confidence["reject_threshold"], configuration.confidence["confirmation_threshold"])
        self.assertLessEqual(configuration.confidence["confirmation_threshold"], configuration.confidence["auto_execute_threshold"])
        self.assertFalse(configuration.model_routing["routing"]["allow_cloud"])

    def test_unknown_field_is_rejected_by_runtime_schema_validation(self) -> None:
        root = Path(__file__).resolve().parents[4]
        with tempfile.TemporaryDirectory() as directory:
            copy = Path(directory)
            for name in ("config", "schemas", "protocols"):
                (copy / name).symlink_to(root / name, target_is_directory=True)
            invalid = (root / "config/intent.yaml").read_text(encoding="utf-8") + "unexpected: true\n"
            (copy / "intent.yaml").write_text(invalid, encoding="utf-8")

            with self.assertRaises(IntentServiceError) as caught:
                IntentConfiguration.load(copy, intent_path=copy / "intent.yaml")

        self.assertEqual(caught.exception.code, 2010)


if __name__ == "__main__":
    unittest.main()
