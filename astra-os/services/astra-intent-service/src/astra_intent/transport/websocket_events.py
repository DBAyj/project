from __future__ import annotations

from collections import deque
from dataclasses import dataclass
from threading import Lock


@dataclass(frozen=True, slots=True)
class IntentEvent:
    event: str
    trace_id: str
    payload: dict[str, object]


class IntentEventBuffer:
    """Bounded event source used by a future WebSocket framing adapter."""

    def __init__(self, capacity: int = 256) -> None:
        self._events: deque[IntentEvent] = deque(maxlen=capacity)
        self._lock = Lock()

    def publish(self, event: IntentEvent) -> None:
        with self._lock:
            self._events.append(event)

    def snapshot(self) -> tuple[IntentEvent, ...]:
        with self._lock:
            return tuple(self._events)
