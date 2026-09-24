#!/usr/bin/env python3
from __future__ import annotations

import json
import sys
from collections import Counter
from pathlib import Path

import jsonschema


ROOT = Path(__file__).resolve().parents[1]
SCHEMA_PATH = ROOT / "protocols/audit/audit-event-v1.schema.json"
REQUIRED_EVENTS = {
    "projection_started",
    "projection_paused",
    "projection_resumed",
    "projection_stopped",
    "projection_denied",
}
SENSITIVE_MARKERS = ("password", "token", "secret", "api_key", "private_key")


def validate_audit_file(path: Path) -> Counter[str]:
    schema = json.loads(SCHEMA_PATH.read_text(encoding="utf-8"))
    validator_type = jsonschema.validators.validator_for(schema)
    validator_type.check_schema(schema)
    validator = validator_type(schema, format_checker=jsonschema.FormatChecker())
    events: Counter[str] = Counter()
    records = 0
    for line_number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), start=1):
        record = json.loads(line)
        validator.validate(record)
        lower_record = json.dumps(record, ensure_ascii=False).lower()
        if any(marker in lower_record for marker in SENSITIVE_MARKERS):
            raise ValueError(f"sensitive marker in audit record {line_number}")
        events[record["event"]] += 1
        records += 1
    if records == 0:
        raise ValueError("audit evidence is empty")
    missing = sorted(REQUIRED_EVENTS - set(events))
    if missing:
        raise ValueError(f"audit evidence missing required events: {', '.join(missing)}")
    return events


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: verify_p1_audit_schema.py <audit-jsonl>")
        return 2
    path = Path(sys.argv[1])
    if not path.is_file():
        print(f"AUDIT_SCHEMA_FAILED: missing {path}")
        return 1
    try:
        events = validate_audit_file(path)
    except (json.JSONDecodeError, jsonschema.ValidationError, ValueError) as error:
        print(f"AUDIT_SCHEMA_FAILED: {error}")
        return 1
    print(
        "AUDIT_SCHEMA_PASSED"
        f" records={sum(events.values())}"
        f" started={events['projection_started']}"
        f" paused={events['projection_paused']}"
        f" resumed={events['projection_resumed']}"
        f" stopped={events['projection_stopped']}"
        f" denied={events['projection_denied']}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
