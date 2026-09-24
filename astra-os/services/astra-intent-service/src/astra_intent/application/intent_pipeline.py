from __future__ import annotations

from dataclasses import asdict, dataclass, replace
from datetime import timedelta
from hashlib import sha256
import json
from pathlib import Path
from time import perf_counter
from typing import Any
from uuid import UUID

from jsonschema import Draft202012Validator, FormatChecker

from astra_intent.adapters.audit_adapter import IntentAuditEmitter
from astra_intent.adapters.model_adapter import IntentModelAdapter
from astra_intent.domain.confirmation import ConfirmationRequest
from astra_intent.domain.errors import IntentError, IntentServiceError
from astra_intent.domain.intent import IntentRequest, IntentSlots
from astra_intent.domain.intent_candidate import IntentCandidate
from astra_intent.domain.intent_result import ExecutionPolicy, IntentResult, ProcessingEvidence
from astra_intent.engines.ambiguity_detector import AmbiguityDetector
from astra_intent.engines.candidate_merger import CandidateMerger
from astra_intent.engines.confidence_evaluator import ConfidenceEvaluator
from astra_intent.engines.confirmation_policy import ConfirmationPolicy
from astra_intent.engines.rule_engine import RuleEngine
from astra_intent.extraction.slot_extractor import SlotExtractor
from astra_intent.infrastructure.cache import IntentCache
from astra_intent.infrastructure.clock import SystemClock
from astra_intent.infrastructure.error_registry import ErrorRegistry
from astra_intent.infrastructure.identifiers import IdentifierFactory
from astra_intent.infrastructure.metrics import IntentMetrics
from astra_intent.security.preprocessor import SecurityPreprocessor


@dataclass(frozen=True, slots=True)
class IntentPipelineComponents:
    security: SecurityPreprocessor
    rules: RuleEngine
    model: IntentModelAdapter
    merger: CandidateMerger
    confidence: ConfidenceEvaluator
    slots: SlotExtractor
    ambiguity: AmbiguityDetector
    policy: ConfirmationPolicy
    cache: IntentCache[IntentResult]
    metrics: IntentMetrics
    audit: IntentAuditEmitter
    errors: ErrorRegistry
    identifiers: IdentifierFactory
    clock: SystemClock


class IntentPipeline:
    def __init__(
        self,
        components: IntentPipelineComponents,
        *,
        request_schema_path: Path,
        confirmation_ttl_seconds: int,
    ) -> None:
        schema = json.loads(request_schema_path.read_text(encoding="utf-8"))
        self._request_validator = Draft202012Validator(schema, format_checker=FormatChecker())
        self._components = components
        self._confirmation_ttl_seconds = confirmation_ttl_seconds

    async def parse(self, value: dict[str, Any]) -> IntentResult:
        started = perf_counter()
        trace_id = self._components.identifiers.new()
        request_id = self._valid_uuid(value.get("request_id")) or self._components.identifiers.new()
        session_id = self._valid_uuid(value.get("session_id"))
        validation_errors = sorted(self._request_validator.iter_errors(value), key=lambda error: list(error.path))
        if validation_errors:
            return self._finish_error(
                request_id,
                trace_id,
                self._validation_error_code(validation_errors),
                started,
                session_id=session_id,
            )
        request = IntentRequest.from_dict(value)
        self._audit("intent_request_received", trace_id, request.request_id, {"locale": request.locale}, session_id=request.session_id)
        try:
            preprocessing = self._components.security.process(request.raw_text)
        except IntentServiceError as error:
            if error.code == 2201:
                self._audit(
                    "prompt_injection_detected",
                    trace_id,
                    request.request_id,
                    {"error_code": error.code},
                    session_id=request.session_id,
                    error_code=error.code,
                )
            return self._finish_error(request.request_id, trace_id, error.code, started, session_id=request.session_id)

        self._audit(
            "input_normalized",
            trace_id,
            request.request_id,
            {"sensitive_data_detected": preprocessing.sensitive_data_detected},
            session_id=request.session_id,
        )
        if preprocessing.sensitive_data_detected:
            self._audit(
                "sensitive_data_detected",
                trace_id,
                request.request_id,
                {"kinds": list(preprocessing.sensitive_data_kinds)},
                session_id=request.session_id,
            )

        cache_key = self._cache_key(preprocessing.safe_text, request)
        if not preprocessing.sensitive_data_detected:
            cached = self._components.cache.get(cache_key)
            if cached is not None:
                duration = (perf_counter() - started) * 1000.0
                result = replace(
                    cached,
                    request_id=request.request_id,
                    trace_id=trace_id,
                    created_at=self._components.clock.iso_now(),
                    processing=replace(cached.processing, cache_hit=True, duration_ms=duration),
                )
                self._audit(
                    "intent_cache_hit",
                    trace_id,
                    request.request_id,
                    {"intent": result.intent},
                    session_id=request.session_id,
                    intent=result.intent,
                    confidence=result.confidence,
                    execution_policy=result.execution_policy.value,
                    duration_ms=duration,
                )
                return self._finish(result, session_id=request.session_id)

        rule_matches = ()
        rule_candidates: tuple[IntentCandidate, ...] = ()
        model_candidates: tuple[IntentCandidate, ...] = ()
        rule_used = False
        model_used = False
        rule_failed = False
        model_failed = False
        try:
            rule_matches = self._components.rules.match(preprocessing.normalized_text, request.locale)
            rule_candidates = tuple(match.candidate for match in rule_matches)
            rule_used = True
            self._audit(
                "rule_engine_completed",
                trace_id,
                request.request_id,
                {"candidate_count": len(rule_candidates)},
                session_id=request.session_id,
            )
        except IntentServiceError:
            rule_failed = True
        try:
            model_candidates = await self._components.model.predict(preprocessing.safe_text, request.locale, request.context)
            model_used = True
            self._audit(
                "local_model_completed",
                trace_id,
                request.request_id,
                {"candidate_count": len(model_candidates)},
                session_id=request.session_id,
            )
        except IntentServiceError:
            model_failed = True
        fallback_used = rule_failed or model_failed
        if rule_failed and model_failed:
            return self._finish_error(
                request.request_id,
                trace_id,
                2004,
                started,
                rule_engine_used=False,
                local_model_used=False,
                fallback_used=True,
                session_id=request.session_id,
            )
        if fallback_used:
            self._audit(
                "intent_fallback_used",
                trace_id,
                request.request_id,
                {"rule_failed": rule_failed, "model_failed": model_failed},
                session_id=request.session_id,
            )

        try:
            candidates = self._components.merger.merge(rule_candidates, model_candidates)
        except Exception:
            return self._finish_error(
                request.request_id,
                trace_id,
                2303,
                started,
                rule_used,
                model_used,
                fallback_used,
                session_id=request.session_id,
            )
        self._audit(
            "candidate_merge_completed",
            trace_id,
            request.request_id,
            {"candidate_count": len(candidates)},
            session_id=request.session_id,
        )
        selected_intent = candidates[0].intent if candidates else "unknown"
        selected_rule = next((match for match in rule_matches if match.candidate.intent == selected_intent), None)
        defaults = selected_rule.default_slots if selected_rule is not None else {}
        required_slots = selected_rule.required_slots if selected_rule is not None else ()
        try:
            extraction = self._components.slots.extract(preprocessing.normalized_text, defaults=defaults)
        except Exception:
            return self._finish_error(
                request.request_id,
                trace_id,
                2304,
                started,
                rule_used,
                model_used,
                fallback_used,
                session_id=request.session_id,
            )
        self._audit(
            "slot_extraction_completed",
            trace_id,
            request.request_id,
            {"slot_names": self._present_slots(extraction.slots)},
            session_id=request.session_id,
        )
        missing = tuple(name for name in required_slots if getattr(extraction.slots, name) is None)
        confidence = candidates[0].confidence if candidates else 0.0
        confidence = self._components.confidence.evaluate(confidence, missing_slot_count=len(missing))
        if candidates and confidence != candidates[0].confidence:
            candidates = (
                IntentCandidate(
                    candidates[0].intent,
                    confidence,
                    candidates[0].source,
                    candidates[0].rule_id,
                    candidates[0].evidence + ("confidence-adjusted",),
                ),
                *candidates[1:],
            )
        ambiguity = self._components.ambiguity.detect(
            preprocessing.normalized_text,
            candidates,
            extraction.slots,
            required_slots=required_slots,
            slot_conflicts=extraction.conflicts,
        )
        if ambiguity.ambiguous:
            self._audit(
                "ambiguity_detected",
                trace_id,
                request.request_id,
                {"reasons": list(ambiguity.reasons)},
                session_id=request.session_id,
                intent=selected_intent,
                confidence=confidence,
            )
        decision = self._components.policy.decide(selected_intent, confidence, request.context, ambiguous=ambiguity.ambiguous)
        confirmation = self._confirmation(request, selected_intent, decision.confirmation_message)
        error_code = None
        if decision.execution_policy is ExecutionPolicy.REJECT:
            error_code = decision.error_code
        elif decision.execution_policy is ExecutionPolicy.ASK_CLARIFICATION:
            error_code = ambiguity.error_code or decision.error_code
        duration = (perf_counter() - started) * 1000.0
        result = IntentResult(
            request_id=request.request_id,
            trace_id=trace_id,
            intent=selected_intent,
            confidence=confidence,
            execution_policy=decision.execution_policy,
            candidates=candidates,
            slots=extraction.slots,
            normalized_text=preprocessing.safe_text,
            processing=ProcessingEvidence(rule_used, model_used, fallback_used, False, duration),
            created_at=self._components.clock.iso_now(),
            ambiguous=ambiguity.ambiguous,
            warnings=("sensitive_data_redacted",) if preprocessing.sensitive_data_detected else (),
            error=self._intent_error(error_code) if error_code is not None else None,
            clarification=ambiguity.clarification,
            confirmation=confirmation,
        )
        if confirmation is not None:
            self._audit(
                "confirmation_required",
                trace_id,
                request.request_id,
                {"intent": selected_intent},
                session_id=request.session_id,
                intent=selected_intent,
                confidence=confidence,
                execution_policy=decision.execution_policy.value,
                duration_ms=duration,
            )
        elif decision.execution_policy is ExecutionPolicy.AUTO_EXECUTE:
            self._audit(
                "intent_auto_executed",
                trace_id,
                request.request_id,
                {"intent": selected_intent},
                session_id=request.session_id,
                intent=selected_intent,
                confidence=confidence,
                execution_policy=decision.execution_policy.value,
                duration_ms=duration,
            )
        elif decision.execution_policy is ExecutionPolicy.REJECT:
            self._audit(
                "intent_rejected",
                trace_id,
                request.request_id,
                {"error_code": error_code},
                session_id=request.session_id,
                intent=selected_intent,
                confidence=confidence,
                execution_policy=decision.execution_policy.value,
                duration_ms=duration,
                error_code=error_code,
            )
        if (
            decision.execution_policy is ExecutionPolicy.AUTO_EXECUTE
            and not preprocessing.sensitive_data_detected
            and not fallback_used
        ):
            self._components.cache.put(cache_key, result)
        return self._finish(result, session_id=request.session_id)

    def error_message(self, code: int) -> str:
        return self._components.errors.lookup(code).message_zh

    def _finish(self, result: IntentResult, *, session_id: str | None) -> IntentResult:
        self._components.metrics.record(
            duration_ms=result.processing.duration_ms,
            failed=result.error is not None and result.execution_policy is ExecutionPolicy.REJECT,
            cache_hit=result.processing.cache_hit,
            fallback_used=result.processing.fallback_used,
        )
        self._audit(
            "intent_request_completed",
            result.trace_id,
            result.request_id,
            result.to_dict(),
            session_id=session_id,
            intent=result.intent,
            confidence=result.confidence,
            execution_policy=result.execution_policy.value,
            duration_ms=result.processing.duration_ms,
            error_code=None if result.error is None else result.error.code,
        )
        return result

    def _finish_error(
        self,
        request_id: str,
        trace_id: str,
        code: int,
        started: float,
        rule_engine_used: bool = False,
        local_model_used: bool = False,
        fallback_used: bool = False,
        *,
        session_id: str | None = None,
    ) -> IntentResult:
        result = self._error_result(
            request_id,
            trace_id,
            code,
            started,
            rule_engine_used,
            local_model_used,
            fallback_used,
        )
        self._audit(
            "intent_service_error",
            trace_id,
            request_id,
            {"error_code": code},
            session_id=session_id,
            intent="unknown",
            confidence=0.0,
            execution_policy=ExecutionPolicy.REJECT.value,
            duration_ms=result.processing.duration_ms,
            error_code=code,
        )
        self._audit(
            "intent_rejected",
            trace_id,
            request_id,
            {"error_code": code},
            session_id=session_id,
            intent="unknown",
            confidence=0.0,
            execution_policy=ExecutionPolicy.REJECT.value,
            duration_ms=result.processing.duration_ms,
            error_code=code,
        )
        return self._finish(result, session_id=session_id)

    def _confirmation(self, request: IntentRequest, action: str, message: str | None) -> ConfirmationRequest | None:
        if message is None:
            return None
        created = self._components.clock.now()
        expires = created + timedelta(seconds=self._confirmation_ttl_seconds)
        return ConfirmationRequest(
            confirmation_id=self._components.identifiers.new(),
            request_id=request.request_id,
            action=action,
            message=message,
            created_at=created.isoformat(timespec="milliseconds").replace("+00:00", "Z"),
            expires_at=expires.isoformat(timespec="milliseconds").replace("+00:00", "Z"),
        )

    def _intent_error(self, code: int) -> IntentError:
        descriptor = self._components.errors.lookup(code)
        return IntentError(descriptor.code, descriptor.message_zh)

    def _error_result(
        self,
        request_id: str,
        trace_id: str,
        code: int,
        started: float,
        rule_engine_used: bool,
        local_model_used: bool,
        fallback_used: bool,
    ) -> IntentResult:
        return IntentResult(
            request_id=request_id,
            trace_id=trace_id,
            intent="unknown",
            confidence=0.0,
            execution_policy=ExecutionPolicy.REJECT,
            candidates=(),
            slots=IntentSlots(),
            normalized_text="",
            processing=ProcessingEvidence(
                rule_engine_used,
                local_model_used,
                fallback_used,
                False,
                (perf_counter() - started) * 1000.0,
            ),
            created_at=self._components.clock.iso_now(),
            error=self._intent_error(code),
        )

    def _audit(
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
    ) -> None:
        self._components.audit.emit(
            event,
            trace_id,
            request_id,
            details,
            session_id=session_id,
            intent=intent,
            confidence=confidence,
            execution_policy=execution_policy,
            duration_ms=duration_ms,
            error_code=error_code,
        )

    @staticmethod
    def _valid_uuid(value: object) -> str | None:
        if not isinstance(value, str):
            return None
        try:
            UUID(value)
        except ValueError:
            return None
        return value

    def _cache_key(self, safe_text: str, request: IntentRequest) -> str:
        value = {
            "text": safe_text,
            "locale": request.locale,
            "context": asdict(request.context),
            "rules_version": self._components.rules.version,
            "model_version": type(self._components.model).__name__,
        }
        return sha256(json.dumps(value, ensure_ascii=False, sort_keys=True).encode("utf-8")).hexdigest()

    @staticmethod
    def _present_slots(slots: IntentSlots) -> list[str]:
        return [name for name, value in asdict(slots).items() if value is not None]

    @staticmethod
    def _validation_error_code(errors: list[Any]) -> int:
        for error in errors:
            path = tuple(error.path)
            if path == ("locale",):
                return 2008
            if path == ("raw_text",) and error.validator == "maxLength":
                return 2203
        return 2009
