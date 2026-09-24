from __future__ import annotations

from dataclasses import dataclass

from astra_intent.domain.errors import IntentServiceError
from astra_intent.extraction.text_normalizer import TextNormalizer

from .injection_detector import InjectionDetector
from .input_limits import InputLimits
from .sensitive_data_filter import SensitiveDataFilter


@dataclass(frozen=True, slots=True)
class SecurityPreprocessingResult:
    normalized_text: str
    safe_text: str
    sensitive_data_detected: bool
    sensitive_data_kinds: tuple[str, ...]
    injection_detected: bool


class SecurityPreprocessor:
    def __init__(
        self,
        *,
        limits: InputLimits | None = None,
        normalizer: TextNormalizer | None = None,
        sensitive_data_filter: SensitiveDataFilter | None = None,
        injection_detector: InjectionDetector | None = None,
    ) -> None:
        self._limits = limits or InputLimits()
        self._normalizer = normalizer or TextNormalizer()
        self._sensitive_data_filter = sensitive_data_filter or SensitiveDataFilter()
        self._injection_detector = injection_detector or InjectionDetector()

    def process(self, text: str) -> SecurityPreprocessingResult:
        self._limits.validate(text)
        normalized = self._normalizer.normalize(text)
        injection = self._injection_detector.detect(normalized)
        redaction = self._sensitive_data_filter.redact(normalized)
        if injection.detected:
            raise IntentServiceError(2201, "Intent request was rejected by prompt-injection screening")
        return SecurityPreprocessingResult(
            normalized_text=normalized,
            safe_text=redaction.redacted_text,
            sensitive_data_detected=redaction.detected,
            sensitive_data_kinds=redaction.kinds,
            injection_detected=False,
        )
