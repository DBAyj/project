from __future__ import annotations

import re
import unicodedata

from astra_intent.domain.errors import IntentServiceError


class InputLimits:
    def __init__(self, *, max_characters: int = 2000, max_repeated_characters: int = 100) -> None:
        self._max_characters = max_characters
        self._repetition = re.compile(rf"(.)\1{{{max_repeated_characters},}}", re.DOTALL)

    def validate(self, text: str) -> None:
        if not text or not text.strip():
            raise IntentServiceError(2009, "Intent request text must not be empty")
        if len(text) > self._max_characters:
            raise IntentServiceError(2203, "Intent request text exceeds the configured limit")
        if any(unicodedata.category(character).startswith("C") for character in text):
            raise IntentServiceError(2202, "Intent request contains a restricted control character")
        if self._repetition.search(text):
            raise IntentServiceError(2202, "Intent request contains excessive character repetition")
