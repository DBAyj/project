from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path

from astra_intent.adapters.audit_adapter import IntentAuditEmitter
from astra_intent.adapters.deterministic_model import DeterministicIntentModel
from astra_intent.adapters.model_adapter import IntentModelAdapter
from astra_intent.application.intent_pipeline import IntentPipeline, IntentPipelineComponents
from astra_intent.engines.ambiguity_detector import AmbiguityDetector
from astra_intent.engines.candidate_merger import CandidateMerger, CandidateMergerConfig
from astra_intent.engines.confidence_evaluator import ConfidenceEvaluator
from astra_intent.engines.confirmation_policy import ConfirmationPolicy, ConfirmationPolicyConfig
from astra_intent.engines.rule_engine import RuleEngine
from astra_intent.extraction.slot_extractor import SlotExtractor
from astra_intent.infrastructure.cache import IntentCache
from astra_intent.infrastructure.clock import SystemClock
from astra_intent.infrastructure.configuration import IntentConfiguration
from astra_intent.infrastructure.error_registry import ErrorRegistry
from astra_intent.infrastructure.identifiers import IdentifierFactory
from astra_intent.infrastructure.metrics import IntentMetrics
from astra_intent.security.input_limits import InputLimits
from astra_intent.security.preprocessor import SecurityPreprocessor


@dataclass(frozen=True, slots=True)
class ServiceContext:
    root: Path
    configuration: IntentConfiguration
    pipeline: IntentPipeline
    clock: SystemClock
    identifiers: IdentifierFactory
    metrics: IntentMetrics
    audit: IntentAuditEmitter

    @classmethod
    def from_repository(
        cls,
        root: Path,
        *,
        rule_engine: RuleEngine | None = None,
        model: IntentModelAdapter | None = None,
        audit_path: Path | None = None,
        clock: SystemClock | None = None,
    ) -> "ServiceContext":
        configuration = IntentConfiguration.load(root)
        merger_config = CandidateMergerConfig(**configuration.intent["candidate_merger"])
        confidence_values = configuration.confidence
        policy_config = ConfirmationPolicyConfig(
            auto_execute_threshold=confidence_values["auto_execute_threshold"],
            confirmation_threshold=confidence_values["confirmation_threshold"],
            reject_threshold=confidence_values["reject_threshold"],
        )
        limits = configuration.intent["limits"]
        active_clock = clock or SystemClock()
        identifiers = IdentifierFactory()
        metrics = IntentMetrics()
        configured_audit_path = Path(configuration.intent["audit"]["path"])
        audit = IntentAuditEmitter(
            audit_path or root / configured_audit_path,
            root / "protocols/intent/intent-event-v1.schema.json",
            identifiers,
            active_clock,
        )
        cache_config = configuration.intent["cache"]
        rules = rule_engine or RuleEngine.from_files(
            root / "config/intent-rules.yaml", root / "schemas/intent-rules.schema.json"
        )
        local_model = model or DeterministicIntentModel.from_rules_file(root / "config/intent-rules.yaml")
        components = IntentPipelineComponents(
            security=SecurityPreprocessor(
                limits=InputLimits(
                    max_characters=limits["max_input_characters"],
                    max_repeated_characters=limits["max_repeated_characters"],
                )
            ),
            rules=rules,
            model=local_model,
            merger=CandidateMerger(merger_config),
            confidence=ConfidenceEvaluator(merger_config),
            slots=SlotExtractor(),
            ambiguity=AmbiguityDetector(delta=confidence_values["ambiguity_delta"]),
            policy=ConfirmationPolicy(policy_config),
            cache=IntentCache(max_entries=cache_config["max_entries"], ttl_seconds=cache_config["ttl_seconds"]),
            metrics=metrics,
            audit=audit,
            errors=ErrorRegistry.from_file(root / "protocols/error-codes.yaml"),
            identifiers=identifiers,
            clock=active_clock,
        )
        pipeline = IntentPipeline(
            components,
            request_schema_path=root / "protocols/intent/intent-request-v2.schema.json",
            confirmation_ttl_seconds=configuration.intent["confirmation"]["ttl_seconds"],
        )
        return cls(root, configuration, pipeline, active_clock, identifiers, metrics, audit)
