#!/usr/bin/env python3
"""Validate the frozen AstraOS P0/P0.5 document baseline."""

from __future__ import annotations

import json
import re
import shutil
import subprocess
import sys
import tempfile
from collections import Counter
from datetime import UTC, datetime
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
REPORT = ROOT / "docs/reports/p0-p05-document-verification.md"
REQUIREMENT = re.compile(r"\b(?:FR|NFR)-[A-Z]+-\d{3}\b")
MODULE = re.compile(r"`(astra-[a-z-]+)`")
MARKDOWN_LINK = re.compile(r"(?<!!)\[[^\]]+\]\(([^)]+)\)")

MODULES = {
    "astra-shell",
    "astra-spatial",
    "astra-ai-runtime",
    "astra-intent",
    "astra-task-engine",
    "astra-model-router",
    "astra-memory",
    "astra-projection",
    "astra-display",
    "astra-input",
    "astra-file",
    "astra-device",
    "astra-media",
    "astra-notification",
    "astra-security",
    "astra-policy",
    "astra-audit",
    "astra-settings",
    "astra-update",
    "astra-runtime",
    "astra-sdk",
    "astra-hal",
    "astra-common",
}
PRIVACY_LEVELS = {
    "PUBLIC",
    "ROOM_ONLY",
    "AUTHORIZED_PERSON",
    "PRIVATE_SCREEN_ONLY",
    "NO_PROJECTION",
}
CONFIG_NAMES = (
    "astra.example",
    "system",
    "display",
    "projection",
    "spatial",
    "ai",
    "privacy",
    "security",
    "logging",
    "update",
    "network",
)
EXPECTED_MODULE_PHASES = {
    "astra-shell": "P1", "astra-display": "P1", "astra-ai-runtime": "P2", "astra-intent": "P2",
    "astra-task-engine": "P2", "astra-model-router": "P2", "astra-memory": "P2", "astra-spatial": "P3",
    "astra-input": "P3", "astra-projection": "P4", "astra-security": "P4", "astra-policy": "P4",
    "astra-audit": "P4", "astra-file": "P5", "astra-device": "P5", "astra-media": "P5",
    "astra-notification": "P5", "astra-settings": "P5", "astra-update": "P5", "astra-runtime": "P6",
    "astra-sdk": "P6", "astra-hal": "P7", "astra-common": "P0",
}
EXPECTED_REQUIREMENT_PHASES = {
    "FR-INTENT-001": "P2", "FR-INTENT-002": "P2", "FR-TASK-001": "P2", "FR-TASK-002": "P2",
    "FR-PROJECTION-001": "P4", "FR-PROJECTION-002": "P4", "FR-DISPLAY-001": "P1",
    "FR-SPATIAL-001": "P3", "FR-AI-001": "P2", "FR-MEMORY-001": "P2", "FR-SECURITY-001": "P4",
    "FR-SECURITY-002": "P4", "FR-AUDIT-001": "P4", "FR-CONFIG-001": "P5", "FR-UPDATE-001": "P5",
    "FR-UPDATE-002": "P5", "FR-RUNTIME-001": "P6", "NFR-PERF-001": "P1", "NFR-SEC-001": "P4",
    "NFR-RELIABILITY-001": "P4", "NFR-PORTABILITY-001": "P7", "NFR-MAINT-001": "P6", "NFR-TEST-001": "P1",
}
ARCHITECTURE_DOCS = tuple(f"docs/architecture/{number:02d}-{name}.md" for number, name in (
    (1, "system-context"), (2, "system-architecture"), (3, "module-design"),
    (4, "process-model"), (5, "ipc-protocol"), (6, "directory-layout"),
    (7, "configuration-standard"), (8, "logging-standard"), (9, "error-code-standard"),
    (10, "security-architecture"), (11, "privacy-model"), (12, "data-model"),
    (13, "ui-design-guideline"), (14, "development-convention"), (15, "git-convention"),
    (16, "versioning-standard"), (17, "testing-strategy"), (18, "update-and-rollback-design"),
    (19, "platform-abstraction"), (20, "future-hardware-architecture"),
))
DIAGRAMS = (
    "astra-system-context.mmd",
    "astra-layered-architecture.mmd",
    "astra-process-model.mmd",
    "astra-data-flow.mmd",
    "astra-security-boundary.mmd",
    "astra-projection-flow.mmd",
    "astra-update-flow.mmd",
)
P05_DOCUMENTS = (
    "docs/product/AstraOS-PRD-v1.0.md",
    "docs/requirements/AstraOS-SRS-v1.0.md",
    "docs/requirements/traceability-matrix.md",
    "docs/design/AstraOS-HLD-v1.0.md",
    "docs/design/AstraOS-LLD-Baseline-v1.0.md",
    "docs/design/AstraOS-Data-Design-v1.0.md",
    "docs/protocols/AstraOS-Protocol-Specification-v1.0.md",
    "docs/ui/AstraOS-UI-UX-Guideline-v1.0.md",
    "docs/sdk/AstraOS-SDK-Design-v1.0.md",
    "docs/sdk/AstraOS-Plugin-Development-v1.0.md",
    "docs/developer/AstraOS-Developer-Guide-v1.0.md",
    "docs/project/risk-register.md",
)


class Verification:
    def __init__(self) -> None:
        self.passes: list[str] = []
        self.failures: list[str] = []

    def require(self, condition: bool, label: str) -> None:
        if condition:
            self.passes.append(label)
        else:
            self.failures.append(label)


def text(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def verify_paths(check: Verification) -> None:
    required = [
        "AGENTS.md", "README.md", "VERSION", "CHANGELOG.md", "CMakeLists.txt", "Makefile",
        *ARCHITECTURE_DOCS,
        *(f"docs/architecture/diagrams/{name}" for name in DIAGRAMS),
        "docs/architecture/adr/ADR-0001-use-qt-for-cross-platform-ui.md",
        "docs/architecture/adr/ADR-0002-use-json-rpc-over-unix-socket.md",
        "docs/architecture/adr/ADR-0003-use-capability-based-security.md",
        *P05_DOCUMENTS,
        "protocols/error-codes.yaml",
        "schemas/protocols/json-rpc-request.schema.json",
        "schemas/protocols/json-rpc-response.schema.json",
        "schemas/protocols/websocket-event.schema.json",
        "protocols/method-registry.yaml",
        "protocols/event-registry.yaml",
        "schemas/plugins/plugin-manifest.schema.json",
    ]
    required.extend(f"config/{name}.yaml" for name in CONFIG_NAMES)
    required.extend(f"schemas/config/{name}.schema.json" for name in CONFIG_NAMES)
    for method in (
        "projection-start", "system-health", "system-shutdown", "event-subscribe",
        "projection-render", "projection-layers-submit", "projection-output-clear", "projection-output-frame", "projection-session-command",
    ):
        required.extend((f"schemas/protocols/methods/{method}.params.schema.json", f"schemas/protocols/methods/{method}.result.schema.json"))
    for method in ("spatial-health", "spatial-state", "spatial-source-start", "spatial-source-stop", "spatial-detect", "spatial-select", "spatial-calibrate", "spatial-observer-set", "spatial-reset"):
        required.extend((f"schemas/protocols/methods/{method}.params.schema.json", f"schemas/protocols/methods/{method}.result.schema.json"))
    required.extend(
        f"schemas/protocols/events/{event}.payload.schema.json"
        for event in (
            "projection-session-changed", "task-state-changed", "system-health-changed",
            "projection-render-state-changed", "projection-safe-cleared", "spatial-state-changed",
        )
    )
    required.extend(("protocols/spatial/spatial-scene-v1.schema.json", "protocols/spatial/observer-pose-v1.schema.json"))
    missing = [path for path in required if not (ROOT / path).exists()]
    check.require(not missing, "required repository paths exist" if not missing else f"missing paths: {', '.join(missing)}")


def verify_modules(check: Verification) -> None:
    found = set(re.findall(r"^\| `(astra-[a-z-]+)` \|", text(ROOT / "docs/architecture/03-module-design.md"), re.MULTILINE))
    check.require(found == MODULES, "23 frozen module names are complete and exact" if found == MODULES else f"module mismatch: {sorted(found ^ MODULES)}")
    phases = dict(re.findall(r"^\| `(astra-[a-z-]+)` \|.*; (P\d+)\. \|$", text(ROOT / "docs/architecture/03-module-design.md"), re.MULTILINE))
    check.require(phases == EXPECTED_MODULE_PHASES, "module implementation phases match the frozen roadmap" if phases == EXPECTED_MODULE_PHASES else f"module phase mismatch: {phases}")


def verify_requirements(check: Verification) -> tuple[int, int]:
    srs_ids = REQUIREMENT.findall(text(ROOT / "docs/requirements/AstraOS-SRS-v1.0.md"))
    matrix_ids = REQUIREMENT.findall(text(ROOT / "docs/requirements/traceability-matrix.md"))
    duplicates = [identifier for identifier, count in Counter(srs_ids).items() if count > 1]
    check.require(not duplicates, "SRS requirement IDs are unique" if not duplicates else f"duplicate SRS requirements: {duplicates}")
    srs_set, matrix_set = set(srs_ids), set(matrix_ids)
    missing = sorted(srs_set - matrix_set)
    extra = sorted(matrix_set - srs_set)
    check.require(not missing and not extra, "all requirements have one traceability destination" if not missing and not extra else f"traceability mismatch missing={missing} extra={extra}")
    phases = dict(re.findall(r"^\| ((?:FR|NFR)-[A-Z]+-\d{3}) \|.*\| (P\d+) \|$", text(ROOT / "docs/requirements/traceability-matrix.md"), re.MULTILINE))
    check.require(phases == EXPECTED_REQUIREMENT_PHASES, "requirement implementation phases match the frozen roadmap" if phases == EXPECTED_REQUIREMENT_PHASES else f"requirement phase mismatch: {phases}")
    return sum(identifier.startswith("FR-") for identifier in srs_set), sum(identifier.startswith("NFR-") for identifier in srs_set)


def verify_error_codes(check: Verification) -> None:
    registry = text(ROOT / "protocols/error-codes.yaml")
    codes = [int(value) for value in re.findall(r"^\s*- code: (\d{4})$", registry, re.MULTILINE)]
    duplicate_codes = [str(code) for code, count in Counter(codes).items() if count > 1]
    valid = bool(codes) and all(1000 <= code <= 9999 for code in codes)
    check.require(valid and not duplicate_codes, "error registry has unique four-digit codes" if valid and not duplicate_codes else f"invalid error codes: {duplicate_codes}")
    ranges = ((1000, 1999), (2000, 2999), (3000, 3999), (4000, 4999), (5000, 5999), (6000, 6999), (7000, 7999), (8000, 8999), (9000, 9999))
    check.require(all(any(low <= code <= high for low, high in ranges) for code in codes), "error codes fit registered ranges")


def parse_yaml_scalar(value: str) -> object:
    if value in ("true", "false"):
        return value == "true"
    if re.fullmatch(r"-?\d+", value):
        return int(value)
    if re.fullmatch(r"-?(?:\d+\.\d*|\d*\.\d+)", value):
        return float(value)
    if len(value) >= 2 and value[0] == value[-1] and value[0] in ("'", '"'):
        return value[1:-1]
    if value and not value.startswith(("[", "{", "-")):
        return value
    raise ValueError("unsupported YAML scalar")


def parse_flat_yaml(source: str) -> dict[str, object]:
    result: dict[str, object] = {}
    for number, raw_line in enumerate(source.splitlines(), start=1):
        if not raw_line.strip() or raw_line.lstrip().startswith("#"):
            continue
        match = re.fullmatch(r"([A-Za-z_][A-Za-z0-9_]*):[ ]*(.*)", raw_line)
        if not match or match.group(1) in result:
            raise ValueError(f"invalid YAML at line {number}")
        result[match.group(1)] = parse_yaml_scalar(match.group(2))
    return result


def parse_one_level_yaml(source: str) -> dict[str, object]:
    """Parse the restricted one-level configuration shape used by P3 spatial."""
    result: dict[str, object] = {}
    section: dict[str, object] | None = None
    for number, raw_line in enumerate(source.splitlines(), start=1):
        if not raw_line.strip() or raw_line.lstrip().startswith("#"):
            continue
        indent = len(raw_line) - len(raw_line.lstrip(" "))
        match = re.fullmatch(r"([A-Za-z_][A-Za-z0-9_]*):[ ]*(.*)", raw_line.lstrip(" "))
        if match is None:
            raise ValueError(f"invalid YAML at line {number}")
        key, value = match.groups()
        if indent == 0:
            if key in result:
                raise ValueError(f"duplicate YAML key at line {number}")
            if value:
                result[key] = parse_yaml_scalar(value)
                section = None
            else:
                section = {}
                result[key] = section
        elif indent == 2 and section is not None and value:
            if key in section:
                raise ValueError(f"duplicate YAML key at line {number}")
            section[key] = parse_yaml_scalar(value)
        else:
            raise ValueError(f"unsupported YAML shape at line {number}")
    return result


def matches_schema(value: object, schema: dict[str, object]) -> bool:
    if "oneOf" in schema:
        choices = schema["oneOf"]
        if not isinstance(choices, list) or sum(matches_schema(value, choice) for choice in choices if isinstance(choice, dict)) != 1:
            return False
    if "not" in schema and isinstance(schema["not"], dict) and matches_schema(value, schema["not"]):
        return False
    if "const" in schema and value != schema["const"]:
        return False
    if "enum" in schema and value not in schema["enum"]:
        return False
    value_type = schema.get("type")
    if value_type == "string" and not isinstance(value, str):
        return False
    if value_type == "integer" and (not isinstance(value, int) or isinstance(value, bool)):
        return False
    if value_type == "boolean" and not isinstance(value, bool):
        return False
    if value_type == "object" or any(key in schema for key in ("properties", "required", "additionalProperties")):
        if not isinstance(value, dict):
            return False
        properties = schema.get("properties", {})
        required = set(schema.get("required", []))
        if not isinstance(properties, dict) or not required.issubset(value):
            return False
        if schema.get("additionalProperties") is False and set(value) - set(properties):
            return False
        for key, item in value.items():
            rule = properties.get(key)
            if isinstance(rule, dict) and not matches_schema(item, rule):
                return False
    if value_type == "array":
        if not isinstance(value, list):
            return False
        item_schema = schema.get("items")
        if isinstance(item_schema, dict) and not all(matches_schema(item, item_schema) for item in value):
            return False
        if schema.get("uniqueItems") is True and len({json.dumps(item, sort_keys=True) for item in value}) != len(value):
            return False
    if isinstance(value, str) and "minLength" in schema and len(value) < schema["minLength"]:
        return False
    if isinstance(value, int) and not isinstance(value, bool) and "minimum" in schema and value < schema["minimum"]:
        return False
    if isinstance(value, str) and "pattern" in schema and not re.search(schema["pattern"], value):
        return False
    return True


def verify_config_schemas(check: Verification) -> None:
    for name in CONFIG_NAMES:
        schema_path = ROOT / f"schemas/config/{name}.schema.json"
        config_path = ROOT / f"config/{name}.yaml"
        try:
            schema = json.loads(text(schema_path))
            if name == "astra.example":
                source = text(config_path)
                top_level_keys = set(re.findall(r"^([A-Za-z_][A-Za-z0-9_]*):\s*$", source, re.MULTILINE))
                required = set(schema["required"])
                valid = top_level_keys == required and all(f"  {key}:" in source for key in ("name", "phone", "default_target", "simulated_authorized", "level", "recent_task_limit"))
                check.require(valid, "valid nested configuration Schema checks: astra.example")
                continue
            if name == "spatial":
                schema = json.loads(text(ROOT / "schemas/spatial-config.schema.json"))
                config = parse_one_level_yaml(text(config_path))
            else:
                config = parse_flat_yaml(text(config_path))
            invalid_unknown = dict(config, unexpected_field="invalid")
            invalid_missing = dict(config)
            invalid_missing.pop(next(iter(schema["required"])))
            invalid_nested = dict(config)
            nested_name = next((key for key, value in config.items() if isinstance(value, dict)), None)
            if nested_name is not None:
                invalid_nested[nested_name] = dict(config[nested_name])
                invalid_nested[nested_name]["unexpected_field"] = "invalid"
            valid = (
                matches_schema(config, schema)
                and not matches_schema(invalid_unknown, schema)
                and not matches_schema(invalid_missing, schema)
                and (nested_name is None or not matches_schema(invalid_nested, schema))
            )
        except (KeyError, OSError, ValueError, json.JSONDecodeError):
            valid = False
        check.require(valid, f"valid and invalid configuration Schema checks: {name}")


def verify_protocol_schemas(check: Verification) -> None:
    try:
        request = json.loads(text(ROOT / "schemas/protocols/json-rpc-request.schema.json"))
        response = json.loads(text(ROOT / "schemas/protocols/json-rpc-response.schema.json"))
        registry = text(ROOT / "protocols/method-registry.yaml")
        methods = re.findall(r"^  - method: ([a-z_]+(?:\.[a-z_]+)+)$", registry, re.MULTILINE)
        schema_paths = re.findall(r"^    (?:params|result)_schema: (schemas/.+\.json)$", registry, re.MULTILINE)
        expected_methods = [
            "projection.start", "system.health", "system.shutdown", "event.subscribe",
            "projection.render", "projection.layers.submit", "projection.output.clear", "projection.output.frame", "projection.session.command",
            "spatial_ui.status", "spatial_ui.components", "spatial_ui.windows", "spatial_ui.focus.status",
            "spatial_ui.layout.status", "spatial_ui.metrics", "spatial_ui.component.create",
            "spatial_ui.task.create", "spatial_ui.task.update", "spatial_ui.component.update",
            "spatial_ui.component.remove", "spatial_ui.window.open", "spatial_ui.window.move", "spatial_ui.window.resize",
            "spatial_ui.window.hide", "spatial_ui.window.show", "spatial_ui.window.target", "spatial_ui.window.restore",
            "spatial_ui.window.close", "spatial_ui.layout.apply", "spatial_ui.layout.reset", "spatial_ui.input", "spatial_ui.interaction.cancel",
            "spatial_ui.focus", "spatial_ui.notification.create", "spatial_ui.notification.clear", "spatial_ui.state.save",
            "spatial_ui.state.load", "spatial_ui.target.lost", "spatial_ui.target.available", "spatial_ui.reset",
            "spatial.health", "spatial.state", "spatial.source.start", "spatial.source.stop", "spatial.detect",
            "spatial.select", "spatial.calibrate", "spatial.observer.set", "spatial.reset",
        ]
        methods_valid = methods == expected_methods and len(schema_paths) == 2 * len(expected_methods)
        schemas_valid = all(
            json.loads(text(ROOT / path)).get("additionalProperties") is False
            or "$ref" in json.loads(text(ROOT / path))
            for path in schema_paths
        )
        method_capabilities = re.findall(r"^    capability: ([a-z_.]+)$", registry, re.MULTILINE)
        event_registry = text(ROOT / "protocols/event-registry.yaml")
        events = re.findall(r"^  - event: ([a-z_]+(?:\.[a-z_]+)+)$", event_registry, re.MULTILINE)
        event_paths = re.findall(r"^    payload_schema: ((?:schemas|protocols)/.+\.json)$", event_registry, re.MULTILINE)
        event_capabilities = re.findall(r"^    subscription_capability: ([a-z_.]+)$", event_registry, re.MULTILINE)
        events_valid = events == [
            "projection.session_changed", "task.state_changed", "system.health_changed",
            "projection.render_state_changed", "projection.safe_cleared", "spatial_ui.event", "spatial.state_changed",
        ] and len(event_paths) == 7 and len(event_capabilities) == 7 and all(json.loads(text(ROOT / path)).get("additionalProperties") is False for path in event_paths)
        plugin_schema = json.loads(text(ROOT / "schemas/plugins/plugin-manifest.schema.json"))
        plugin_valid = plugin_schema.get("type") == "object" and plugin_schema.get("additionalProperties") is False and len(plugin_schema.get("required", [])) == 8
        request_properties = request.get("properties", {})
        security_context = request_properties.get("security_context", {})
        response_valid = "oneOf" in response and "trace_id" in request_properties and "security_context" in request.get("required", []) and security_context.get("required") == ["capability_token"] and len(method_capabilities) == len(expected_methods)
        check.require(methods_valid and schemas_valid and response_valid and events_valid and plugin_valid, "public method, event, and plugin schemas are complete")
    except (OSError, json.JSONDecodeError):
        check.require(False, "public method, event, and plugin schemas are complete")


def verify_protocol_authorization_examples(check: Verification) -> None:
    try:
        request_schema = json.loads(text(ROOT / "schemas/protocols/json-rpc-request.schema.json"))
        response_schema = json.loads(text(ROOT / "schemas/protocols/json-rpc-response.schema.json"))
        projection_schema = json.loads(text(ROOT / "schemas/protocols/methods/projection-start.params.schema.json"))
        subscribe_schema = json.loads(text(ROOT / "schemas/protocols/methods/event-subscribe.params.schema.json"))
        valid_request = {
            "jsonrpc": "2.0", "id": "request-uuid", "trace_id": "trace-uuid", "method": "projection.start",
            "params": {"scene_id": "scene-uuid", "target": "desk", "privacy_level": "PUBLIC"},
            "security_context": {"capability_token": "token-uuid"},
        }
        missing_token = dict(valid_request)
        missing_token.pop("security_context")
        empty_token = dict(valid_request, security_context={"capability_token": ""})
        unknown_field = dict(valid_request, caller_debug=True)
        valid_projection = {"scene_id": "scene-uuid", "target": "desk", "privacy_level": "PUBLIC"}
        invalid_projection = dict(valid_projection, target="unknown")
        valid_subscription = {"event": "projection.session_changed"}
        invalid_subscription = {"event": "undeclared.event"}
        valid_success = {"jsonrpc": "2.0", "id": "request-uuid", "result": {}}
        valid_error = {"jsonrpc": "2.0", "id": "request-uuid", "error": {"code": 5001, "message": "Permission denied", "data": {}}}
        invalid_response = dict(valid_success, error=valid_error["error"])
        valid = (
            matches_schema(valid_request, request_schema)
            and not matches_schema(missing_token, request_schema)
            and not matches_schema(empty_token, request_schema)
            and not matches_schema(unknown_field, request_schema)
            and matches_schema(valid_projection, projection_schema)
            and not matches_schema(invalid_projection, projection_schema)
            and matches_schema(valid_subscription, subscribe_schema)
            and not matches_schema(invalid_subscription, subscribe_schema)
            and matches_schema(valid_success, response_schema)
            and matches_schema(valid_error, response_schema)
            and not matches_schema(invalid_response, response_schema)
        )
    except (OSError, json.JSONDecodeError):
        valid = False
    check.require(valid, "IPC authorization and allow/deny Schema examples pass")


def verify_privacy_and_protocols(check: Verification) -> None:
    privacy_sources = (
        ROOT / "docs/architecture/11-privacy-model.md",
        ROOT / "docs/requirements/AstraOS-SRS-v1.0.md",
        ROOT / "schemas/config/projection.schema.json",
    )
    consistent = all(level in text(path) for path in privacy_sources for level in PRIVACY_LEVELS)
    check.require(consistent, "privacy levels are consistent across architecture, requirements, and schema")
    protocol_text = text(ROOT / "docs/protocols/AstraOS-Protocol-Specification-v1.0.md")
    check.require(all(term in protocol_text for term in ("JSON-RPC 2.0", "WebSocket", "Unix Domain Sockets")), "protocol names are consistent")


def verify_links(check: Verification) -> None:
    broken: list[str] = []
    for path in ROOT.glob("docs/**/*.md"):
        if "docs/backups" in path.as_posix():
            continue
        for destination in MARKDOWN_LINK.findall(text(path)):
            if destination.startswith(("http://", "https://", "mailto:", "#")):
                continue
            target = destination.split("#", 1)[0]
            if target and not (path.parent / target).resolve().exists():
                broken.append(f"{path.relative_to(ROOT)} -> {destination}")
    check.require(not broken, "internal Markdown links resolve" if not broken else f"broken links: {'; '.join(broken)}")


def verify_mermaid(check: Verification) -> None:
    runner = shutil.which("npx")
    if runner is None:
        check.require(False, "Mermaid source renders with the Mermaid CLI")
        return
    errors: list[str] = []
    scratch_root = ROOT / "runtime/tmp"
    scratch_root.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(dir=scratch_root) as output_directory:
        for name in DIAGRAMS:
            result = subprocess.run(
                [runner, "--yes", "@mermaid-js/mermaid-cli", "-i", str(ROOT / "docs/architecture/diagrams" / name), "-o", str(Path(output_directory) / f"{name}.svg")],
                capture_output=True,
                text=True,
                check=False,
            )
            if result.returncode != 0:
                errors.append(f"{name}: {result.stderr.strip() or result.stdout.strip()}")
    check.require(not errors, "Mermaid source renders with the Mermaid CLI" if not errors else f"invalid Mermaid sources: {errors}")


def verify_no_placeholders(check: Verification) -> None:
    offenders: list[str] = []
    for path in ROOT.glob("docs/**/*.md"):
        if "docs/backups" not in path.as_posix() and re.search(r"\b(?:TBD|TODO)\b", text(path), re.IGNORECASE):
            offenders.append(path.relative_to(ROOT).as_posix())
    check.require(not offenders, "no unexplained placeholders in managed documents" if not offenders else f"placeholder text in: {offenders}")


def write_report(check: Verification, functional: int, non_functional: int) -> None:
    architecture_count = len(list((ROOT / "docs/architecture").glob("[0-9][0-9]-*.md")))
    risk_count = len(re.findall(r"\| R-\d{3} \|", text(ROOT / "docs/project/risk-register.md")))
    status = "PASS" if not check.failures else "FAIL"
    lines = [
        "# AstraOS P0 + P0.5 Document Verification",
        "",
        f"- Generated: {datetime.now(UTC).isoformat(timespec='seconds')}",
        f"- Result: `{status}`",
        f"- Architecture documents: {architecture_count}",
        f"- Mermaid diagrams: {len(DIAGRAMS)}",
        f"- ADRs: {len(list((ROOT / 'docs/architecture/adr').glob('ADR-*.md')))}",
        f"- Frozen modules: {len(MODULES)}",
        "- Processes: 7",
        "- Protocol transports: 3 (JSON-RPC, WebSocket, HAL)",
        f"- Functional requirements: {functional}",
        f"- Non-functional requirements: {non_functional}",
        f"- Risks: {risk_count}",
        "",
        "## Checks",
        "",
    ]
    lines.extend(f"- PASS: {item}" for item in check.passes)
    lines.extend(f"- FAIL: {item}" for item in check.failures)
    if not check.failures:
        lines.extend(("", "## Conclusion", "", "The P0 and P0.5 documentation baseline is internally consistent according to automated checks."))
    report = "\n".join(lines) + "\n"
    if REPORT.exists():
        existing = text(REPORT)
        normalized_existing = re.sub(r"^- Generated: .+$", "- Generated: <stable>", existing, flags=re.MULTILINE)
        normalized_report = re.sub(r"^- Generated: .+$", "- Generated: <stable>", report, flags=re.MULTILINE)
        if normalized_existing == normalized_report:
            return
    REPORT.write_text(report, encoding="utf-8")


def main() -> int:
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    if not REPORT.exists():
        REPORT.write_text("# AstraOS P0 + P0.5 Document Verification\n\nValidation running.\n", encoding="utf-8")
    check = Verification()
    verify_paths(check)
    verify_modules(check)
    functional, non_functional = verify_requirements(check)
    verify_error_codes(check)
    verify_config_schemas(check)
    verify_protocol_schemas(check)
    verify_protocol_authorization_examples(check)
    verify_privacy_and_protocols(check)
    verify_links(check)
    verify_mermaid(check)
    verify_no_placeholders(check)
    write_report(check, functional, non_functional)
    print(text(REPORT), end="")
    return 0 if not check.failures else 1


if __name__ == "__main__":
    sys.exit(main())
