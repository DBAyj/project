from __future__ import annotations

from dataclasses import asdict, dataclass
from typing import Any


@dataclass(frozen=True, slots=True)
class IntentContext:
    projection_state: str
    current_space: str
    current_privacy_level: str
    current_model_id: str | None


@dataclass(frozen=True, slots=True)
class IntentSlots:
    target_space: str | None = None
    model_id: str | None = None
    privacy_level: str | None = None
    zoom_factor: float | None = None
    rotation_direction: str | None = None
    rotation_degrees: float | None = None
    display_target: str | None = None
    confirmation_response: str | None = None

    def to_dict(self) -> dict[str, Any]:
        return asdict(self)


@dataclass(frozen=True, slots=True)
class IntentRequest:
    request_id: str
    session_id: str
    user_id: str
    locale: str
    raw_text: str
    context: IntentContext
    client_name: str
    client_version: str
    requested_at: str
    schema_version: str = "2.0"

    @classmethod
    def from_dict(cls, value: dict[str, Any]) -> "IntentRequest":
        context = value["current_context"]
        client = value["client"]
        return cls(
            request_id=value["request_id"],
            session_id=value["session_id"],
            user_id=value["user_id"],
            locale=value["locale"],
            raw_text=value["raw_text"],
            context=IntentContext(**context),
            client_name=client["name"],
            client_version=client["version"],
            requested_at=value["requested_at"],
            schema_version=value["schema_version"],
        )

    def to_dict(self) -> dict[str, Any]:
        return {
            "schema_version": self.schema_version,
            "request_id": self.request_id,
            "session_id": self.session_id,
            "user_id": self.user_id,
            "locale": self.locale,
            "raw_text": self.raw_text,
            "current_context": asdict(self.context),
            "client": {"name": self.client_name, "version": self.client_version},
            "requested_at": self.requested_at,
        }
