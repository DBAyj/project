from __future__ import annotations

import json
from pathlib import Path
from threading import Lock

from jsonschema import Draft202012Validator, FormatChecker

from astra_intent.infrastructure.clock import SystemClock
from astra_intent.infrastructure.identifiers import IdentifierFactory


class IntentAuditEmitter:
    def __init__(
        self,
        path: Path,
        schema_path: Path,
        identifiers: IdentifierFactory,
        clock: SystemClock,
    ) -> None:
        schema = json.loads(schema_path.read_text(encoding="utf-8"))
        self._validator = Draft202012Validator(schema, format_checker=FormatChecker())
        self._path = path
        self._identifiers = identifiers
        self._clock = clock
        self._lock = Lock()

    def emit(
        self,
        event: str,
        trace_id: str,
        request_id: str,
        details: dict[str, object],
        *,
        session_id: str | None = None,
        intent: str | None = None,
        confidence: float | None = None,
        execution_policy: str | None = None,
        duration_ms: float | None = None,
        error_code: int | None = None,
    ) -> bool:
        processing = details.get("processing")
        if isinstance(processing, dict) and duration_ms is None:
            duration_value = processing.get("duration_ms")
            if isinstance(duration_value, (int, float)) and not isinstance(duration_value, bool):
                duration_ms = float(duration_value)
        if intent is None and isinstance(details.get("intent"), str):
            intent = str(details["intent"])
        if confidence is None:
            confidence_value = details.get("confidence")
            if isinstance(confidence_value, (int, float)) and not isinstance(confidence_value, bool):
                confidence = float(confidence_value)
        if execution_policy is None and isinstance(details.get("execution_policy"), str):
            execution_policy = str(details["execution_policy"])
        if error_code is None:
            direct_error = details.get("error_code")
            nested_error = details.get("error")
            if isinstance(direct_error, int) and not isinstance(direct_error, bool):
                error_code = direct_error
            elif isinstance(nested_error, dict) and isinstance(nested_error.get("code"), int):
                error_code = int(nested_error["code"])
        result = "success"
        level = "INFO"
        if event == "intent_service_error":
            result = "error"
            level = "ERROR"
        elif event in {"prompt_injection_detected", "confirmation_rejected", "intent_rejected"} or error_code is not None:
            result = "rejected"
            level = "WARN"
        elif event in {"ambiguity_detected", "intent_fallback_used"}:
            level = "WARN"
        record = {
            "schema_version": "1.0",
            "event_id": self._identifiers.new(),
            "timestamp": self._clock.iso_now(),
            "level": level,
            "service": "astra-intent-service",
            "event": event,
            "trace_id": trace_id,
            "request_id": request_id,
            "session_id": session_id,
            "intent": intent,
            "confidence": confidence,
            "execution_policy": execution_policy,
            "duration_ms": round(max(0.0, duration_ms or 0.0), 3),
            "result": result,
            "error_code": error_code,
            "details": details,
        }
        if list(self._validator.iter_errors(record)):
            return False
        try:
            with self._lock:
                self._path.parent.mkdir(parents=True, exist_ok=True)
                with self._path.open("a", encoding="utf-8") as stream:
                    stream.write(json.dumps(record, ensure_ascii=False, separators=(",", ":")) + "\n")
            return True
        except OSError:
            return False
