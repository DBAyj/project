from __future__ import annotations

from dataclasses import dataclass, fields
import re

from astra_intent.domain.intent import IntentSlots

from .model_extractor import ModelExtractor
from .privacy_extractor import PrivacyExtractor
from .target_extractor import TargetExtractor


@dataclass(frozen=True, slots=True)
class SlotExtraction:
    slots: IntentSlots
    conflicts: tuple[str, ...] = ()


class SlotExtractor:
    _degrees = re.compile(r"(\d+(?:\.\d+)?)\s*度")

    def __init__(self) -> None:
        self._target = TargetExtractor()
        self._privacy = PrivacyExtractor()
        self._model = ModelExtractor()

    def extract(self, text: str, *, defaults: dict[str, object] | None = None) -> SlotExtraction:
        privacy = self._privacy.extract(text)
        target = self._target.extract(text)
        values: dict[str, object | None] = {
            "target_space": target,
            "model_id": self._model.extract(text),
            "privacy_level": privacy.privacy_level,
            "zoom_factor": self._zoom(text),
            "rotation_direction": self._rotation_direction(text),
            "rotation_degrees": self._rotation_degrees(text),
            "display_target": self._display_target(text, target),
            "confirmation_response": self._confirmation(text),
        }
        allowed = {field.name for field in fields(IntentSlots)}
        for name, value in (defaults or {}).items():
            if name in allowed and values[name] is None:
                values[name] = value
        return SlotExtraction(IntentSlots(**values), privacy.matches if len(privacy.matches) > 1 else ())

    @staticmethod
    def _zoom(text: str) -> float | None:
        comparable = text.casefold()
        if "放大两倍" in comparable or "放大2倍" in comparable or "zoom in 2x" in comparable:
            return 2.0
        if "缩小到一半" in comparable or "half size" in comparable:
            return 0.5
        if "放大" in comparable or "zoom in" in comparable:
            return 1.25
        if "缩小" in comparable or "zoom out" in comparable:
            return 0.8
        return None

    @staticmethod
    def _rotation_direction(text: str) -> str | None:
        comparable = text.casefold()
        if "逆时针" in comparable or "counterclockwise" in comparable:
            return "counterclockwise"
        if "顺时针" in comparable or "clockwise" in comparable:
            return "clockwise"
        if "向左" in comparable or "左转" in comparable or "rotate left" in comparable:
            return "left"
        if "向右" in comparable or "右转" in comparable or "rotate right" in comparable:
            return "right"
        return None

    def _rotation_degrees(self, text: str) -> float | None:
        match = self._degrees.search(text)
        return float(match.group(1)) if match else None

    @staticmethod
    def _display_target(text: str, target: str | None) -> str | None:
        comparable = text.casefold()
        if target == "phone_screen" or "手机显示" in comparable or "phone display" in comparable:
            return "phone_screen"
        if target in {"desk", "wall", "projection_screen"} or "投影" in comparable or "projection" in comparable:
            return "projection_screen"
        return None

    @staticmethod
    def _confirmation(text: str) -> str | None:
        comparable = text.casefold().strip()
        if any(term in comparable for term in ("确认", "继续执行", "confirm", "yes")):
            return "confirm"
        if any(term in comparable for term in ("拒绝", "不要执行", "reject", "no")):
            return "reject"
        return None
