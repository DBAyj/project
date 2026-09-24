from __future__ import annotations

import unittest

from astra_intent.infrastructure.cache import IntentCache
from astra_intent.infrastructure.metrics import IntentMetrics


class IntentCacheTests(unittest.TestCase):
    def test_hit_expiry_and_lru_capacity_are_deterministic(self) -> None:
        now = [100.0]
        cache: IntentCache[str] = IntentCache(max_entries=2, ttl_seconds=10, monotonic=lambda: now[0])
        cache.put("a", "first")
        cache.put("b", "second")
        self.assertEqual(cache.get("a"), "first")
        cache.put("c", "third")
        self.assertIsNone(cache.get("b"))
        now[0] = 111.0
        self.assertIsNone(cache.get("a"))
        self.assertEqual(cache.size(), 0)


class IntentMetricsTests(unittest.TestCase):
    def test_snapshot_reports_counts_and_observed_latency(self) -> None:
        metrics = IntentMetrics()
        metrics.record(duration_ms=10.0, failed=False, cache_hit=False, fallback_used=False)
        metrics.record(duration_ms=30.0, failed=True, cache_hit=True, fallback_used=True)

        snapshot = metrics.snapshot()

        self.assertEqual(snapshot["requests_total"], 2)
        self.assertEqual(snapshot["errors_total"], 1)
        self.assertEqual(snapshot["cache_hits_total"], 1)
        self.assertEqual(snapshot["fallback_total"], 1)
        self.assertEqual(snapshot["latency_ms"]["average"], 20.0)
        self.assertEqual(snapshot["latency_ms"]["p95"], 30.0)


if __name__ == "__main__":
    unittest.main()
