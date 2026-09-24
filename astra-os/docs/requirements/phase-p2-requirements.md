# Phase P2 Requirements

## Scope

P2 replaces the in-process general intent simulator with an independent, portable `astra-intent-service`. The service accepts strict versioned requests, performs security preprocessing before classification, combines configuration-driven rules with a deterministic local model, extracts typed slots, evaluates confidence and ambiguity, applies confirmation policy, and emits auditable results. P1 projection state, privacy policy, and the minimal Shell safety fallback remain authoritative.

## Requirements

| ID | Requirement | Acceptance |
| --- | --- | --- |
| P2-INTENT-001 | Produce a versioned structured intent result. | Result includes candidate sources, confidence, execution policy, ambiguity, slots, processing evidence, and trace IDs. |
| P2-INTENT-002 | Support the frozen P2 intent vocabulary and multilingual examples. | Configuration and deterministic-model tests cover Chinese and English phrases, negation, and ambiguity. |
| P2-SEC-001 | Validate and normalize input before classification. | Empty, oversized, control-character, repetition, and injection inputs are rejected; sensitive values are redacted from audit. |
| P2-RULE-001 | Load and validate configuration-driven intent rules. | Exact, keyword, synonym, template, priority, locale, default-slot, enabled, and negation behavior pass tests. |
| P2-MODEL-001 | Provide a deterministic local model adapter. | Equal input/context produces equal candidates without external calls or model downloads. |
| P2-FUSION-001 | Merge rule and model candidates deterministically. | Agreement bonus, conflict penalty, missing-slot penalty, and ambiguity delta are configurable and tested. |
| P2-SLOT-001 | Extract the required P2 slots without fabrication. | Target, model, privacy, zoom, rotation, display, and confirmation slots are typed or null. |
| P2-CONFIRM-001 | Require confirmation or clarification for risky or ambiguous operations. | Pending confirmations expire, cannot be replayed, and never execute before acceptance. |
| P2-IPC-001 | Serve JSON-RPC over a project Unix socket. | Stale socket handling, duplicate startup failure, strict requests, health, and cleanup pass integration tests. |
| P2-HTTP-001 | Expose development HTTP health, readiness, intent, confirmation, supported-intent, and metrics routes. | Routes are disabled outside explicit development mode and pass HTTP integration tests. |
| P2-FALLBACK-001 | Preserve safe operation through component failure. | Either engine can operate alone; dual failure rejects; Shell retains stop, hide, and status rules when the service is unavailable. |
| P2-SHELL-001 | Present intent analysis and confirmation state in Phone Display. | Auto-execute, confirmation, clarification, rejection, and offline-degraded states are visible; Projection Display has no AI dependency. |
| P2-AUDIT-001 | Emit complete redacted audit events for the intent pipeline. | Every required event is parseable, trace-correlated, schema-valid, and free of raw sensitive values. |
| P2-PERF-001 | Meet P2 host performance targets. | Measured rule P95 <=20 ms, pipeline P95 <=100 ms, health P95 <=20 ms, 20 concurrent requests without errors, startup <=3 seconds. |
| P2-STABILITY-001 | Complete 10,000 deterministic intent requests without resource leakage. | No crash, deadlock, unclosed socket, invalid audit record, or sustained memory growth. |

## Exclusions

P2 does not call paid or public cloud models, download large models, train models, add multi-agent orchestration or long-term memory, process camera/voice/spatial input, control real projection hardware, build Linux images, or enter Phase P3.
