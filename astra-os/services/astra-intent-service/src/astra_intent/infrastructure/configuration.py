from __future__ import annotations

from dataclasses import dataclass
import json
from pathlib import Path
from typing import Any

from jsonschema import Draft202012Validator, FormatChecker
import yaml

from astra_intent.domain.errors import IntentServiceError


@dataclass(frozen=True, slots=True)
class IntentConfiguration:
    intent: dict[str, Any]
    security: dict[str, Any]
    model_routing: dict[str, Any]
    rules: dict[str, Any]

    @property
    def service(self) -> dict[str, Any]:
        return self.intent["service"]

    @property
    def confidence(self) -> dict[str, float]:
        return self.intent["confidence"]

    @classmethod
    def load(cls, root: Path, *, intent_path: Path | None = None) -> "IntentConfiguration":
        paths = (
            (intent_path or root / "config/intent.yaml", root / "schemas/intent-config.schema.json"),
            (root / "config/intent-security.yaml", root / "schemas/intent-security.schema.json"),
            (root / "config/model-routing.yaml", root / "schemas/model-routing.schema.json"),
            (root / "config/intent-rules.yaml", root / "schemas/intent-rules.schema.json"),
        )
        documents = tuple(cls._load_validated(document, schema) for document, schema in paths)
        confidence = documents[0]["confidence"]
        if not (
            confidence["reject_threshold"]
            <= confidence["confirmation_threshold"]
            <= confidence["auto_execute_threshold"]
        ):
            raise IntentServiceError(2010, "Intent confidence thresholds must be monotonic")
        return cls(*documents)

    @staticmethod
    def _load_validated(document_path: Path, schema_path: Path) -> dict[str, Any]:
        try:
            document = yaml.safe_load(document_path.read_text(encoding="utf-8"))
            schema = json.loads(schema_path.read_text(encoding="utf-8"))
            errors = sorted(
                Draft202012Validator(schema, format_checker=FormatChecker()).iter_errors(document),
                key=lambda error: list(error.path),
            )
            if errors:
                path = ".".join(str(part) for part in errors[0].path) or "$"
                raise IntentServiceError(2010, f"Configuration {document_path.name} is invalid at {path}: {errors[0].message}")
            if not isinstance(document, dict):
                raise IntentServiceError(2010, f"Configuration {document_path.name} must be an object")
            return document
        except IntentServiceError:
            raise
        except (OSError, ValueError, TypeError, yaml.YAMLError, json.JSONDecodeError) as error:
            raise IntentServiceError(2010, f"Configuration {document_path.name} cannot be loaded: {error}") from error
