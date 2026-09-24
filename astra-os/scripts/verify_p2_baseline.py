#!/usr/bin/env python3
from __future__ import annotations

import json
from pathlib import Path
import re
import sys

import yaml


ROOT = Path(__file__).resolve().parents[1]


def require(condition: bool, message: str, failures: list[str]) -> None:
    print(("PASS: " if condition else "FAIL: ") + message)
    if not condition:
        failures.append(message)


def test_count(path: Path) -> int:
    if not path.exists():
        return 0
    return sum(int(value) for value in re.findall(r"Ran (\d+) tests?", path.read_text(encoding="utf-8", errors="replace")))


def main() -> int:
    failures: list[str] = []
    rules = yaml.safe_load((ROOT / "config/intent-rules.yaml").read_text(encoding="utf-8"))
    intents = {rule["intent"] for rule in rules["rules"]}
    generation = rules["example_generation"]
    core_count = len(intents - {"unknown"})
    sample_counts = {
        "chinese": core_count * len(generation["positive_zh_templates"]),
        "english": core_count * len(generation["positive_en_templates"]),
        "ambiguous": core_count * len(generation["ambiguous_templates"]),
        "negative": core_count * len(generation["negative_templates"]),
    }
    require(len(intents) == 30, "30 supported intents registered", failures)
    require(rules is not None and (ROOT / "config/intent.yaml").read_text(encoding="utf-8").find("version: 0.2.0-alpha.1") >= 0, "P2 service version is 0.2.0-alpha.1", failures)
    require(sample_counts == {"chinese": 290, "english": 87, "ambiguous": 87, "negative": 87}, "rule example coverage", failures)
    for relative in (
        "protocols/intent/intent-request-v2.schema.json",
        "protocols/intent/intent-result-v2.schema.json",
        "protocols/intent/intent-candidate-v1.schema.json",
        "protocols/intent/intent-confirmation-v1.schema.json",
        "protocols/intent/intent-event-v1.schema.json",
        "schemas/intent-config.schema.json",
        "schemas/intent-rules.schema.json",
        "schemas/intent-security.schema.json",
        "schemas/model-routing.schema.json",
    ):
        require((ROOT / relative).is_file(), f"schema exists: {relative}", failures)
    for relative in (
        "docs/requirements/phase-p2-requirements.md",
        "docs/reports/p2-preflight-audit.md",
        "docs/reports/p2-build-report.md",
        "docs/reports/p2-test-report.md",
        "docs/reports/p2-security-report.md",
        "docs/reports/p2-performance-report.md",
        "docs/reports/p2-verification-report.md",
        "tests/p2-e2e/test_p2_live_service.py",
    ):
        require((ROOT / relative).is_file(), f"P2 baseline artifact exists: {relative}", failures)
    source_paths = [ROOT / "services/astra-intent-service", ROOT / "apps/astra-shell/src/clients", ROOT / "apps/astra-shell/qml/phone"]
    placeholders: list[str] = []
    for base in source_paths:
        for path in base.rglob("*"):
            if path.is_file() and path.suffix in {".py", ".cpp", ".h", ".qml"}:
                if re.search(r"\b(?:TODO|TBD)\b", path.read_text(encoding="utf-8", errors="replace")):
                    placeholders.append(str(path.relative_to(ROOT)))
    require(not placeholders, f"no unexplained TODO/TBD placeholders: {placeholders}", failures)
    test_root = ROOT / "runtime/tmp/p2-tests"
    counts = {
        "unit": test_count(test_root / "unit.log"),
        "contract": test_count(test_root / "contract.log") + test_count(test_root / "schema-contract.log"),
        "integration": test_count(test_root / "integration.log"),
        "security": test_count(test_root / "security.log"),
    }
    require(all(value > 0 for value in counts.values()), f"non-empty P2 test groups: {counts}", failures)
    performance_path = ROOT / "runtime/tmp/p2-performance.json"
    stability_path = ROOT / "runtime/tmp/p2-stability.json"
    require(performance_path.is_file(), "performance evidence exists", failures)
    require(stability_path.is_file(), "stability evidence exists", failures)
    live_path = ROOT / "runtime/tmp/p2-live-acceptance.json"
    require(live_path.is_file(), "live service acceptance evidence exists", failures)
    summary = {
        "status": "PASSED" if not failures else "FAILED",
        "supported_intents": len(intents),
        "samples": sample_counts,
        "tests": counts,
        "failures": failures,
    }
    (ROOT / "runtime/tmp/p2-verification-summary.json").write_text(
        json.dumps(summary, indent=2, ensure_ascii=False) + "\n", encoding="utf-8"
    )
    return 0 if not failures else 1


if __name__ == "__main__":
    raise SystemExit(main())
