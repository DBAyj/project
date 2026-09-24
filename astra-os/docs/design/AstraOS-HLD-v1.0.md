# AstraOS High-Level Design v1.0

## Architecture and Module Relationships

AstraOS uses a layered, multi-process system. `astra-shell` presents QML interfaces and requests work; `astra-ai-service` transforms user input into policy-aware task plans; `astra-core-service` coordinates display, input, data, settings, and runtime; projection, security/policy, audit, and update have dedicated processes. The canonical module dependency matrix is [03 Module Design](../architecture/03-module-design.md).

## Deployment and Communication Model

The development host is macOS Apple Silicon only. The deployable target is ARM64 Linux LTS with Wayland and Vulkan or OpenGL ES. Unix sockets host local JSON-RPC request/response, WebSocket carries subscribed events, and future shared memory carries high-frequency graphics. HAL adapters are the only hardware boundary. Development HTTP is disabled unless an explicit development configuration enables it.

## Security and Data Flow

An input becomes an `Intent`, then a `Task`; each privileged task step calls `astra-security` and `astra-policy`, then an owning service. The service emits typed results for Phone UI or policy-approved Projection UI and submits audit events. Cloud AI is a separate egress path evaluated by model routing and policy before leaving the device. Diagrams: [data flow](../architecture/diagrams/astra-data-flow.mmd), [security boundary](../architecture/diagrams/astra-security-boundary.mmd), [projection flow](../architecture/diagrams/astra-projection-flow.mmd).

## Exceptions, Reliability, and Performance

JSON-RPC errors use the central four-digit registry. A shell failure is isolated; AI failure preserves base display/settings; projection failure closes external output; audit uses bounded local buffering; update writes only an inactive partition. P1 establishes device-specific measurable budgets rather than inventing performance values. Trace, request, session, and audit identifiers support diagnosis.

## Extensibility, Platform Adaptation, and Tradeoffs

Apps, Web, WASM, and AI Skills enter through `astra-runtime` and SDK contracts, not direct service access. Platform adapters isolate macOS host paths/graphics from Linux Wayland/drivers. Qt was selected for portable UI, JSON-RPC over Unix sockets for first-generation local IPC, and capability security for explicit authority; see the three accepted ADRs in [architecture decisions](../architecture/adr/).
