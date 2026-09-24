# Changelog

## Unreleased - Phase P5 baseline remediation

- Added the independent capability-protected Spatial UI Service and portable `astra-ui` component, window, layout, focus, input, interaction, task, notification, privacy, state, animation, and accessibility modules.
- Added strict Spatial UI schemas, error codes, RPC registry entries, 11 deterministic fixtures, Shell clients/models, and shared Phone/Projection QML workspaces.
- Added P5 unit, contract, QML, integration, accessibility, security, performance, stability, Metal, screenshot, run/stop, and release-gate evidence.
- Integrated the P2 and P3 source histories so their real services and regression gates can participate in the P5 release chain.

## 0.4.0-alpha.1 - 2026-07-16

- Reconciled the verified P2 and P3 histories into the P4 release lineage.
- Retained the P4 projection service, render graph, geometry correction, privacy masking, deterministic visual fixtures, Metal evidence, performance, stability, and safe-clear gates.
- Preserved fixture-only P4 spatial claims; physical projector and physical real-space calibration evidence are not claimed.

## 0.3.0-alpha.1 - 2026-07-15

- Added the P3 portable spatial service with deterministic frame sources, OpenCV planar candidates, manual calibration, coordinate mapping, session-only anchors, simulated observer state, JSON-RPC, audit, and Shell target-loss safety.
- Added P3 strict protocol, configuration, public method/event, image-pipeline performance, stability, live run/stop, and P1/P2 regression gates through `make p3-verify`.

## 0.2.0-alpha.1 - 2026-07-14

- Added the independent, offline-first `astra-intent-service` with JSON-RPC 2.0 over Unix Socket and capability-protected development HTTP/WebSocket endpoints.
- Added strict P2 request, result, candidate, confirmation, event, configuration, routing, security, and rule schemas.
- Added input normalization, sensitive-data redaction, prompt-injection signals, configuration-driven rules, deterministic local classification, slot extraction, candidate fusion, ambiguity detection, and confirmation policy.
- Added bounded TTL/LRU caching, metrics, structured JSONL intent audit, engine fallback, dual-engine safe rejection, and the Shell minimum safe offline rules.
- Added the Astra Shell intent client, AI analysis card, confirmation controls, and P1 projection-state integration without adding an AI dependency to Projection Display.
- Added measured 1,000-request performance, 20-request concurrency, 10,000-request stability, C++/QML regression, contract, security, audit, and transport coverage.

## 0.1.0-alpha.1 - 2026-07-14

- Established the P0 architecture freeze and P0.5 system-design documentation baseline.
- Added machine-checkable protocol, configuration, and documentation-baseline artifacts.
- Added the P1 dual-window Qt simulator, deterministic intent controls, projection privacy simulation, structured audit records, and P1 contract/QML/stability coverage.
- Closed P1.1 release-gate findings with an isolated projection-policy service, centralized error registry, operation trace propagation, runtime configuration validation, Phone configuration warnings, accessibility metadata, and QML warning checks.
- Added verified Metal QRhi diagnostics, durable project-scoped simulator startup, actual dual-window loading, pointer-interaction tests, audit-Schema validation, and the `make p1-release-gate` command.
