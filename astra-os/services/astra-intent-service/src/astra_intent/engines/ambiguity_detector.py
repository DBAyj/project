from __future__ import annotations

from dataclasses import dataclass

from astra_intent.domain.intent import IntentSlots
from astra_intent.domain.intent_candidate import IntentCandidate
from astra_intent.domain.intent_result import Clarification


@dataclass(frozen=True, slots=True)
class AmbiguityDecision:
    ambiguous: bool
    error_code: int | None = None
    reasons: tuple[str, ...] = ()
    clarification: Clarification | None = None


class AmbiguityDetector:
    _intent_labels = {
        "start_projection": "启动投影",
        "stop_projection": "停止投影",
        "show_phone_display": "在手机显示",
        "show_projection_display": "在投影显示",
    }

    def __init__(self, *, delta: float = 0.10) -> None:
        self._delta = delta

    def detect(
        self,
        text: str,
        candidates: tuple[IntentCandidate, ...],
        slots: IntentSlots,
        *,
        required_slots: tuple[str, ...] = (),
        slot_conflicts: tuple[str, ...] = (),
    ) -> AmbiguityDecision:
        comparable = text.casefold()
        if slot_conflicts:
            return AmbiguityDecision(
                True,
                2006,
                ("privacy-conflict",),
                Clarification("检测到互斥的隐私要求，请选择一个隐私等级。", tuple(slot_conflicts)),
            )
        if self._contains_start_and_stop(comparable):
            return AmbiguityDecision(
                True,
                2006,
                ("mutually-exclusive-actions",),
                Clarification("你希望启动投影还是停止投影？", ("启动投影", "停止投影")),
            )
        if self._contains_phone_and_projection(comparable):
            return AmbiguityDecision(
                True,
                2006,
                ("display-target-conflict",),
                Clarification("你希望内容显示在手机还是投影屏？", ("手机显示", "投影显示")),
            )
        if ("那个" in comparable and "那里" in comparable) or "put that there" in comparable:
            return AmbiguityDecision(
                True,
                2104,
                ("unresolved-references",),
                Clarification("请补充模型和目标位置。", ("重新描述任务",)),
            )
        missing = tuple(name for name in required_slots if getattr(slots, name) is None)
        if missing:
            labels = {"model_id": "模型", "target_space": "位置", "privacy_level": "隐私等级"}
            names = "和".join(labels.get(name, name) for name in missing)
            return AmbiguityDecision(
                True,
                2104,
                ("missing-required-slots",),
                Clarification(f"请补充{names}。", ("重新描述任务",)),
            )
        if len(candidates) >= 2 and candidates[0].confidence - candidates[1].confidence < self._delta:
            options = tuple(self._intent_labels.get(candidate.intent, candidate.intent) for candidate in candidates[:2])
            return AmbiguityDecision(
                True,
                2006,
                ("close-candidates",),
                Clarification("请确认你希望执行哪项操作？", options),
            )
        return AmbiguityDecision(False)

    @staticmethod
    def _contains_start_and_stop(text: str) -> bool:
        starts = ("打开", "开始", "启动", "start")
        stops = ("关闭", "停止", "结束", "stop", "close")
        return any(term in text for term in starts) and any(term in text for term in stops)

    @staticmethod
    def _contains_phone_and_projection(text: str) -> bool:
        phone = ("手机", "phone")
        projection = ("投影", "外部", "projection")
        return any(term in text for term in phone) and any(term in text for term in projection)
