# AstraOS Low-Level Design Baseline v1.0

## Uniform Module Template

Every implementation module must maintain these sections before implementation: objective; boundary and forbidden responsibilities; components/classes; state machine; public interfaces; inputs and outputs; data structures; error handling; concurrency model; security checks; log/audit events; health check; and test points. The sections inherit the frozen names and dependencies in [03 Module Design](../architecture/03-module-design.md).

## Baseline Component Patterns

| Module group | Components and state-machine baseline | Concurrency and security baseline |
| --- | --- | --- |
| `astra-shell`, `astra-display` | View-model, scene coordinator, target state `UNAVAILABLE/READY/ACTIVE`; QML renders declared state only. | UI thread owns presentation; no privileged decision. |
| `astra-intent`, `astra-ai-runtime`, `astra-model-router`, `astra-memory` | Parser, context store, route evaluator; request `RECEIVED/CLASSIFIED/ROUTED/COMPLETED/FAILED`. | Bounded work queue; classified data and policy approval before cloud. |
| `astra-task-engine` | Planner, executor, compensator; `PENDING/RUNNING/PAUSED/CANCELLED/SUCCEEDED/FAILED/ROLLED_BACK`. | Serialized transition per task; capability check per privileged step. |
| `astra-spatial`, `astra-input`, `astra-hal` | Coordinate mapper, anchor registry, input normalizer, device adapter; device `DISCOVERED/READY/UNAVAILABLE`. | Real-time samples isolated from business queues; camera/sensor access capability-bound. |
| `astra-projection` | Session manager and output gate; `REQUESTED/VALIDATED/ACTIVE/STOPPING/CLOSED/DENIED`. | One state owner per target; policy result is mandatory before surface creation. |
| `astra-file`, `astra-device`, `astra-media`, `astra-notification`, `astra-settings` | Service facade, validated command handler, durable repository where applicable. | Per-request trace scope; security context is validated at service boundary. |
| `astra-security`, `astra-policy`, `astra-audit` | Token validator, policy evaluator, append-only writer; decision `PENDING/ALLOW/DENY`. | Privileged process; serialized integrity writes and no caller bypass. |
| `astra-update` | Package verifier, slot manager, boot verifier; `DISCOVERED/VERIFIED/STAGED/BOOTED/PROMOTED/ROLLED_BACK`. | Exclusive update lock; signature and compatibility checks before inactive-slot write. |
| `astra-runtime`, `astra-sdk`, `astra-common` | Sandbox broker, generated contract facade, portable primitive library. | Runtime calls carry capability context; common has no product dependency. |

## Interface and Test Contract

Each public method has a schema, method version, typed result/error, timeout, idempotency classification, log event, audit classification, and contract test. Each state transition has allow, deny, retry, cancellation, and recovery tests where applicable. Details expand only through an ADR-backed implementation design in the owning phase.
