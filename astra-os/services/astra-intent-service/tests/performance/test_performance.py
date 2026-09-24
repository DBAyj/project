from __future__ import annotations

import asyncio
from concurrent.futures import ThreadPoolExecutor
import json
from pathlib import Path
import resource
import statistics
import sys
import tempfile
from time import perf_counter
import unittest
from uuid import uuid4

from astra_intent.application.intent_service import IntentService
from astra_intent.engines.rule_engine import RuleEngine


def percentile(values: list[float], fraction: float) -> float:
    ordered = sorted(values)
    index = max(0, min(len(ordered) - 1, int((len(ordered) - 1) * fraction)))
    return ordered[index]


def rss_megabytes() -> float:
    value = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss
    return value / (1024 * 1024) if sys.platform == "darwin" else value / 1024


class IntentPerformanceTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.root = Path(__file__).resolve().parents[4]

    @staticmethod
    def request(text: str) -> dict[str, object]:
        return {
            "schema_version": "2.0",
            "request_id": str(uuid4()),
            "session_id": str(uuid4()),
            "user_id": "local-user",
            "locale": "zh-CN",
            "raw_text": text,
            "current_context": {
                "projection_state": "IDLE",
                "current_space": "development_workspace",
                "current_privacy_level": "PUBLIC",
                "current_model_id": "demo-device",
            },
            "client": {"name": "astra-shell", "version": "0.2.0-alpha.1"},
            "requested_at": "2026-07-14T13:40:00Z",
        }

    def test_measured_performance_targets(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            started = perf_counter()
            service = IntentService.from_repository(self.root, audit_path=Path(directory) / "performance.jsonl")
            startup_seconds = perf_counter() - started
            rule_engine = RuleEngine.from_files(
                self.root / "config/intent-rules.yaml", self.root / "schemas/intent-rules.schema.json"
            )

            rule_latencies: list[float] = []
            for _ in range(1000):
                before = perf_counter()
                matches = rule_engine.match("把设备模型投到桌面上", "zh-CN")
                rule_latencies.append((perf_counter() - before) * 1000)
                self.assertEqual(matches[0].candidate.intent, "project_3d_model")

            async def sequential() -> tuple[list[float], list[float]]:
                full: list[float] = []
                for index in range(1000):
                    before = perf_counter()
                    result = await service.parse(self.request(f"把设备模型投到桌面上 {index}"))
                    full.append((perf_counter() - before) * 1000)
                    self.assertNotEqual(result["execution_policy"], "REJECT")
                cached: list[float] = []
                for _ in range(1000):
                    before = perf_counter()
                    result = await service.parse(self.request("查看系统状态"))
                    cached.append((perf_counter() - before) * 1000)
                    self.assertEqual(result["intent"], "show_system_status")
                return full, cached

            rss_before = rss_megabytes()
            full_latencies, cache_latencies = asyncio.run(sequential())
            rss_after = rss_megabytes()

            health_latencies: list[float] = []
            for _ in range(1000):
                before = perf_counter()
                self.assertEqual(service.health()["status"], "healthy")
                health_latencies.append((perf_counter() - before) * 1000)

            def concurrent_call(index: int) -> dict[str, object]:
                return asyncio.run(service.parse(self.request(f"帮我投影这个模型 {index}")))

            concurrent_started = perf_counter()
            with ThreadPoolExecutor(max_workers=20) as executor:
                concurrent_results = list(executor.map(concurrent_call, range(20)))
            concurrent_seconds = perf_counter() - concurrent_started
            concurrent_errors = sum(result["execution_policy"] == "REJECT" for result in concurrent_results)
            report = {
                "startup_seconds": round(startup_seconds, 6),
                "rule_engine": self._latency_report(rule_latencies),
                "full_pipeline": self._latency_report(full_latencies),
                "cache_pipeline": self._latency_report(cache_latencies),
                "health": self._latency_report(health_latencies),
                "concurrency": {
                    "workers": 20,
                    "errors": concurrent_errors,
                    "throughput_rps": round(20 / concurrent_seconds, 3),
                },
                "memory": {
                    "rss_before_mb": round(rss_before, 3),
                    "rss_after_mb": round(rss_after, 3),
                    "growth_mb": round(max(0.0, rss_after - rss_before), 3),
                },
            }
            output = self.root / "runtime/tmp/p2-performance.json"
            output.parent.mkdir(parents=True, exist_ok=True)
            output.write_text(json.dumps(report, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")

            self.assertLessEqual(report["rule_engine"]["p95_ms"], 20.0)
            self.assertLessEqual(report["full_pipeline"]["p95_ms"], 100.0)
            self.assertLessEqual(report["health"]["p95_ms"], 20.0)
            self.assertLessEqual(startup_seconds, 3.0)
            self.assertEqual(concurrent_errors, 0)
            self.assertLessEqual(report["memory"]["growth_mb"], 64.0)

    @staticmethod
    def _latency_report(values: list[float]) -> dict[str, float]:
        return {
            "average_ms": round(statistics.fmean(values), 3),
            "p50_ms": round(percentile(values, 0.50), 3),
            "p95_ms": round(percentile(values, 0.95), 3),
            "p99_ms": round(percentile(values, 0.99), 3),
            "max_ms": round(max(values), 3),
        }


if __name__ == "__main__":
    unittest.main()
