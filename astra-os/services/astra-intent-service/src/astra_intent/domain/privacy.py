from __future__ import annotations

from enum import StrEnum


class PrivacyLevel(StrEnum):
    PUBLIC = "PUBLIC"
    ROOM_ONLY = "ROOM_ONLY"
    AUTHORIZED_PERSON = "AUTHORIZED_PERSON"
    PRIVATE_SCREEN_ONLY = "PRIVATE_SCREEN_ONLY"
    NO_PROJECTION = "NO_PROJECTION"
