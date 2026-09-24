from __future__ import annotations

from collections import OrderedDict
from collections.abc import Callable
from threading import Lock
from time import monotonic
from typing import Generic, TypeVar


Value = TypeVar("Value")


class IntentCache(Generic[Value]):
    def __init__(
        self,
        *,
        max_entries: int,
        ttl_seconds: float,
        monotonic: Callable[[], float] = monotonic,
    ) -> None:
        self._max_entries = max_entries
        self._ttl_seconds = ttl_seconds
        self._monotonic = monotonic
        self._values: OrderedDict[str, tuple[float, Value]] = OrderedDict()
        self._lock = Lock()

    def get(self, key: str) -> Value | None:
        with self._lock:
            entry = self._values.get(key)
            if entry is None:
                return None
            expires_at, value = entry
            if self._monotonic() >= expires_at:
                del self._values[key]
                return None
            self._values.move_to_end(key)
            return value

    def put(self, key: str, value: Value) -> None:
        with self._lock:
            self._values[key] = (self._monotonic() + self._ttl_seconds, value)
            self._values.move_to_end(key)
            while len(self._values) > self._max_entries:
                self._values.popitem(last=False)

    def clear(self) -> None:
        with self._lock:
            self._values.clear()

    def size(self) -> int:
        with self._lock:
            now = self._monotonic()
            expired = [key for key, (expires_at, _) in self._values.items() if now >= expires_at]
            for key in expired:
                del self._values[key]
            return len(self._values)
