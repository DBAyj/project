from __future__ import annotations

from dataclasses import replace
from datetime import datetime
from pathlib import Path
from typing import Any

from astra_intent import __version__
from astra_intent.adapters.model_adapter import IntentModelAdapter
from astra_intent.application.service_context import ServiceContext
from astra_intent.domain.confirmation import ConfirmationRequest, ConfirmationStatus
from astra_intent.domain.errors import IntentServiceError
from astra_intent.engines.rule_engine import RuleEngine
from astra_intent.infrastructure.clock import SystemClock


class IntentService:
    def __init__(self, context: ServiceContext) -> None:
        self._context = context
        self._confirmations: dict[str, ConfirmationRequest] = {}
        self._confirmation_context: dict[str, tuple[str, str, str | None, str, float, str]] = {}

    @classmethod
    def from_repository(
        cls,
        root: Path,
        *,
        rule_engine: RuleEngine | None = None,
        model: IntentModelAdapter | None = None,
        audit_path: Path | None = None,
        clock: SystemClock | None = None,
    ) -> "IntentService":
        return cls(
            ServiceContext.from_repository(
                root,
                rule_engine=rule_engine,
                model=model,
                audit_path=audit_path,
                clock=clock,
            )
        )

    async def parse(self, value: dict[str, Any]) -> dict[str, Any]:
        result = await self._context.pipeline.parse(value)
        if result.confirmation is not None:
            self._confirmations[result.confirmation.confirmation_id] = result.confirmation
            session_id = value.get("session_id") if isinstance(value.get("session_id"), str) else None
            self._confirmation_context[result.confirmation.confirmation_id] = (
                result.trace_id,
                result.request_id,
                session_id,
                result.intent,
                result.confidence,
                result.execution_policy.value,
            )
        return result.to_dict()

    def health(self) -> dict[str, object]:
        return {
            "status": "healthy",
            "service": "astra-intent-service",
            "version": __version__,
            "rule_engine": "healthy",
            "local_model": "healthy",
            "cache": "healthy",
        }

    def ready(self) -> dict[str, object]:
        return {"ready": True, "service": "astra-intent-service", "version": __version__}

    def supported_intents(self) -> tuple[str, ...]:
        return tuple(dict.fromkeys(rule["intent"] for rule in self._context.configuration.rules["rules"]))

    def error_message(self, code: int) -> str:
        return self._context.pipeline.error_message(code)

    def transport_configuration(self) -> dict[str, object]:
        return dict(self._context.configuration.intent["transport"])

    def metrics(self) -> dict[str, object]:
        return self._context.metrics.snapshot()

    def confirm(self, confirmation_id: str) -> dict[str, Any]:
        confirmation = self._pending_confirmation(confirmation_id)
        accepted = replace(confirmation, status=ConfirmationStatus.ACCEPTED)
        self._confirmations[confirmation_id] = accepted
        self._audit_confirmation("confirmation_accepted", accepted)
        return accepted.to_dict()

    def reject(self, confirmation_id: str) -> dict[str, Any]:
        confirmation = self._pending_confirmation(confirmation_id)
        rejected = replace(confirmation, status=ConfirmationStatus.REJECTED)
        self._confirmations[confirmation_id] = rejected
        self._audit_confirmation("confirmation_rejected", rejected)
        return rejected.to_dict()

    def _audit_confirmation(self, event: str, confirmation: ConfirmationRequest) -> None:
        trace_id, request_id, session_id, intent, confidence, execution_policy = self._confirmation_context[
            confirmation.confirmation_id
        ]
        self._context.audit.emit(
            event,
            trace_id,
            request_id,
            confirmation.to_dict(),
            session_id=session_id,
            intent=intent,
            confidence=confidence,
            execution_policy=execution_policy,
        )

    def _pending_confirmation(self, confirmation_id: str) -> ConfirmationRequest:
        confirmation = self._confirmations.get(confirmation_id)
        if confirmation is None or confirmation.status is not ConfirmationStatus.PENDING:
            raise IntentServiceError(2102, "Confirmation is missing, expired, or already consumed")
        expires = datetime.fromisoformat(confirmation.expires_at.replace("Z", "+00:00"))
        if self._context.clock.now() >= expires:
            self._confirmations[confirmation_id] = replace(confirmation, status=ConfirmationStatus.EXPIRED)
            raise IntentServiceError(2102, "Confirmation has expired")
        return confirmation
