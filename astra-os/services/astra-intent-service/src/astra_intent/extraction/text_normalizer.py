from __future__ import annotations

import re
import unicodedata


class TextNormalizer:
    """Create the stable text representation consumed by intent engines."""

    _terminal_punctuation = re.compile(r"[。！？!?；;，,\.]+$")
    _whitespace = re.compile(r"\s+")

    def normalize(self, text: str) -> str:
        normalized = unicodedata.normalize("NFKC", text)
        normalized = self._whitespace.sub(" ", normalized).strip().lower()
        return self._terminal_punctuation.sub("", normalized).strip()
