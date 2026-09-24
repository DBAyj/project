from __future__ import annotations

from typing import Protocol, runtime_checkable

from astra_intent.domain.intent import IntentContext
from astra_intent.domain.intent_candidate import IntentCandidate


@runtime_checkable
class IntentModelAdapter(Protocol):
    async def predict(
        self,
        text: str,
        locale: str,
        context: IntentContext,
    ) -> tuple[IntentCandidate, ...]: ...
