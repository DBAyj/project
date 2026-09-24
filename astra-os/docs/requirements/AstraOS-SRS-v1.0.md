# AstraOS Software Requirements Specification v1.0

## Scope, Terms, and Roles

The system is AstraOS running as a multi-process ARM64 Linux target with a macOS development host. A **capability** is a scoped, expiring authorization; **policy** is a contextual decision; **projection session** is a managed external-output interval; **privacy level** is one of `PUBLIC`, `ROOM_ONLY`, `AUTHORIZED_PERSON`, `PRIVATE_SCREEN_ONLY`, or `NO_PROJECTION`.

Roles are Local User, System Service, Application or AI Skill, Device Adapter, and Operations Maintainer. The system context is defined in [01 System Context](../architecture/01-system-context.md).

## Functional Requirements

| ID | Requirement | Owner module |
| --- | --- | --- |
| FR-INTENT-001 | The system shall convert user text or transcript into a versioned `Intent` with kind, parameters, confidence, and ambiguity state. | astra-intent |
| FR-INTENT-002 | The system shall request clarification rather than execute an ambiguous privileged intent. | astra-intent |
| FR-TASK-001 | The system shall represent a task as ordered steps with pending, running, paused, cancelled, succeeded, failed, and rolled-back states. | astra-task-engine |
| FR-TASK-002 | The system shall support retry, cancellation, pause, resume, and documented compensation for eligible task steps. | astra-task-engine |
| FR-PROJECTION-001 | The system shall create a projection session only after capability and privacy-policy approval. | astra-projection |
| FR-PROJECTION-002 | The system shall prevent `PRIVATE_SCREEN_ONLY` and `NO_PROJECTION` objects from external output. | astra-policy |
| FR-DISPLAY-001 | The system shall distinguish a private phone target from an external projection target. | astra-display |
| FR-SPATIAL-001 | The system shall represent scenes, anchors, observer pose, and privacy-classified scene objects. | astra-spatial |
| FR-AI-001 | The system shall route each model invocation to a local or cloud model only after policy evaluation. | astra-model-router |
| FR-MEMORY-001 | The system shall isolate sensitive memory records and support deletion according to retention policy. | astra-memory |
| FR-SECURITY-001 | The system shall deny a privileged action without a valid scoped capability. | astra-security |
| FR-SECURITY-002 | The system shall record a security-context decision for every elevated grant and denial. | astra-security |
| FR-AUDIT-001 | The system shall record auditable events for AI calls, projection operations, permission denials, and updates. | astra-audit |
| FR-CONFIG-001 | The system shall validate versioned YAML configuration against JSON Schema and reject unknown fields. | astra-settings |
| FR-UPDATE-001 | The system shall verify package signature and compatibility before staging an update to an inactive partition. | astra-update |
| FR-UPDATE-002 | The system shall roll back to the prior version after boot or health-check failure. | astra-update |
| FR-RUNTIME-001 | The system shall mediate application and AI Skill service access through capability-checked APIs. | astra-runtime |

## Non-Functional Requirements

| ID | Requirement |
| --- | --- |
| NFR-PERF-001 | The system shall expose measurable latency, rendering, and recovery telemetry before setting device-specific performance budgets. |
| NFR-SEC-001 | The system shall use deny-by-default, least-privilege, scoped and expiring capabilities for privileged operations. |
| NFR-RELIABILITY-001 | A shell, AI, or projection process failure shall not terminate unrelated core services; projection failure shall close external output. |
| NFR-PORTABILITY-001 | Core services and protocols shall run on ARM64 Linux without AppKit, SwiftUI, macOS-only services, or hardcoded user paths. |
| NFR-MAINT-001 | Public contracts shall be schema-defined, versioned, traceable, and compatible according to documented negotiation rules. |
| NFR-TEST-001 | Each module shall expose health checks and have applicable contract, failure, and acceptance tests. |

## Interface, Data, Safety, Reliability, Compatibility, Maintainability, and Testability

Local requests use JSON-RPC 2.0 over Unix sockets; events use WebSocket; development HTTP is explicitly non-production. Interfaces, request IDs, timeouts, retries, idempotency, and negotiation are specified in [Protocol Specification](../protocols/AstraOS-Protocol-Specification-v1.0.md). The logical data model and storage rules are in [Data Design](../design/AstraOS-Data-Design-v1.0.md). Security and privacy requirements are in [10 Security Architecture](../architecture/10-security-architecture.md) and [11 Privacy Model](../architecture/11-privacy-model.md). Reliability uses isolated processes, health checks, local audit buffering, and rollback design.

## Acceptance Traceability

The authoritative requirement-to-design-to-test mapping is [traceability matrix](traceability-matrix.md). No requirement is accepted without an owning module, design reference, future implementation phase, and test type.
