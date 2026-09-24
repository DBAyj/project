from __future__ import annotations

from dataclasses import dataclass


@dataclass(frozen=True, slots=True)
class IntentError:
    code: int
    message: str

    def to_dict(self) -> dict[str, object]:
        return {"code": self.code, "message": self.message}


class IntentServiceError(RuntimeError):
    def __init__(self, code: int, message: str) -> None:
        super().__init__(message)
        self.code = code
        self.message = message
