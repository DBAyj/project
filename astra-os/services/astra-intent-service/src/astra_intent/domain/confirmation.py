from __future__ import annotations

from dataclasses import dataclass
from enum import StrEnum
from typing import Any


class ConfirmationStatus(StrEnum):
    PENDING = "PENDING"
    ACCEPTED = "ACCEPTED"
    REJECTED = "REJECTED"
    EXPIRED = "EXPIRED"


@dataclass(frozen=True, slots=True)
class ConfirmationRequest:
    confirmation_id: str
    request_id: str
    action: str
    message: str
    created_at: str
    expires_at: str
    status: ConfirmationStatus = ConfirmationStatus.PENDING

    def to_dict(self) -> dict[str, Any]:
        return {
            "schema_version": "1.0",
            "confirmation_id": self.confirmation_id,
            "request_id": self.request_id,
            "action": self.action,
            "message": self.message,
            "status": self.status.value,
            "created_at": self.created_at,
            "expires_at": self.expires_at,
        }
