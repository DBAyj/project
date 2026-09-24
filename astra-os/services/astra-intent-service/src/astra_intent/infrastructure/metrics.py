from __future__ import annotations

from math import ceil
from threading import Lock


class IntentMetrics:
    def __init__(self) -> None:
        self._requests = 0
        self._errors = 0
        self._cache_hits = 0
        self._fallbacks = 0
        self._latencies: list[float] = []
        self._lock = Lock()

    def record(self, *, duration_ms: float, failed: bool, cache_hit: bool, fallback_used: bool) -> None:
        with self._lock:
            self._requests += 1
            self._errors += int(failed)
            self._cache_hits += int(cache_hit)
            self._fallbacks += int(fallback_used)
            self._latencies.append(max(0.0, duration_ms))

    def snapshot(self) -> dict[str, object]:
        with self._lock:
            ordered = sorted(self._latencies)
            average = sum(ordered) / len(ordered) if ordered else 0.0
            p95 = ordered[max(0, ceil(len(ordered) * 0.95) - 1)] if ordered else 0.0
            return {
                "requests_total": self._requests,
                "errors_total": self._errors,
                "cache_hits_total": self._cache_hits,
                "fallback_total": self._fallbacks,
                "latency_ms": {"average": round(average, 3), "p95": round(p95, 3)},
            }
