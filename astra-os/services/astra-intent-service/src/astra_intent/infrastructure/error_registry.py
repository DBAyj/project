from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path

import yaml


@dataclass(frozen=True, slots=True)
class ErrorDescriptor:
    code: int
    symbol: str
    message_zh: str
    message_en: str
    module: str
    severity: str
    retryable: bool
    audit_required: bool


class ErrorRegistry:
    def __init__(self, descriptors: tuple[ErrorDescriptor, ...]) -> None:
        codes = [descriptor.code for descriptor in descriptors]
        symbols = [descriptor.symbol for descriptor in descriptors]
        if len(codes) != len(set(codes)) or len(symbols) != len(set(symbols)):
            raise ValueError("Error registry contains duplicate codes or symbols")
        self._descriptors = descriptors
        self._by_code = {descriptor.code: descriptor for descriptor in descriptors}

    @classmethod
    def from_file(cls, path: Path) -> "ErrorRegistry":
        document = yaml.safe_load(path.read_text(encoding="utf-8"))
        descriptors = tuple(
            ErrorDescriptor(
                code=value["code"],
                symbol=value["name"],
                message_zh=value.get("message_zh", value["message"]),
                message_en=value["message"],
                module=value["module"],
                severity=value.get("severity", "ERROR"),
                retryable=bool(value.get("retryable", False)),
                audit_required=bool(value.get("audit_required", True)),
            )
            for value in document["codes"]
        )
        return cls(descriptors)

    def all(self) -> tuple[ErrorDescriptor, ...]:
        return self._descriptors

    def lookup(self, code: int) -> ErrorDescriptor:
        return self._by_code.get(code, self._by_code[9001])
