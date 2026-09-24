from __future__ import annotations

from dataclasses import dataclass


@dataclass(frozen=True, slots=True)
class PrivacyExtraction:
    privacy_level: str | None
    matches: tuple[str, ...]


class PrivacyExtractor:
    _levels = (
        ("NO_PROJECTION", ("禁止投影", "不要显示到外面", "disable projection", "no projection")),
        ("PRIVATE_SCREEN_ONLY", ("只在手机上显示", "不要投到外面", "phone screen only", "private screen only")),
        ("AUTHORIZED_PERSON", ("只有授权人员", "授权人员能看", "authorized person")),
        ("ROOM_ONLY", ("仅当前房间", "房间里的人", "room only")),
        ("PUBLIC", ("公开展示", "给大家", "所有人都可以看", "privacy public", "public display")),
    )

    def extract(self, text: str) -> PrivacyExtraction:
        comparable = text.casefold()
        matches = tuple(level for level, phrases in self._levels if any(phrase in comparable for phrase in phrases))
        return PrivacyExtraction(matches[0] if len(matches) == 1 else None, matches)
