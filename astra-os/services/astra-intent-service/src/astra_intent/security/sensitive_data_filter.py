from __future__ import annotations

from dataclasses import dataclass
import re
from typing import Callable, Match


@dataclass(frozen=True, slots=True)
class RedactionResult:
    redacted_text: str
    detected: bool
    kinds: tuple[str, ...]


class SensitiveDataFilter:
    _patterns: tuple[tuple[str, re.Pattern[str], str | Callable[[Match[str]], str]], ...] = (
        (
            "email",
            re.compile(
                r"(?<![A-Za-z0-9._%+-])([A-Za-z0-9._%+-]{2,})@([A-Za-z0-9.-]+\.[A-Za-z]{2,})(?![A-Za-z0-9.-])"
            ),
            lambda match: f"{match.group(1)[:2]}***@{match.group(2)}",
        ),
        ("phone", re.compile(r"(?<!\d)(1\d{2})\d{4}(\d{4})(?!\d)"), r"\1****\2"),
        ("identity_card", re.compile(r"(?<!\d)(\d{6})\d{8}(\d{3}[0-9Xx])(?!\w)"), r"\1********\2"),
        ("bank_card", re.compile(r"(?<!\d)(\d{4})\d{8,11}(\d{4})(?!\d)"), r"\1********\2"),
        (
            "api_key",
            re.compile(r"(?<![A-Za-z0-9_-])(?:sk|ak)-[A-Za-z0-9_-]{6,}(?![A-Za-z0-9_-])", re.IGNORECASE),
            "sk-****redacted",
        ),
        (
            "bearer_token",
            re.compile(r"(?<![A-Za-z0-9_-])Bearer\s+[A-Za-z0-9._~+/=-]+", re.IGNORECASE),
            "Bearer ****redacted",
        ),
        (
            "password",
            re.compile(r"(?i)(password|passwd|pwd|密码)\s*[:=：]\s*([^\s,，;；]+)"),
            lambda match: f"{match.group(1)}: ****redacted",
        ),
    )

    def redact(self, text: str) -> RedactionResult:
        redacted = text
        kinds: list[str] = []
        for kind, pattern, replacement in self._patterns:
            redacted, count = pattern.subn(replacement, redacted)
            if count:
                kinds.append(kind)
        return RedactionResult(redacted, bool(kinds), tuple(kinds))
