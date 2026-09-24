# P2 Verification Report

## Scope And Runtime

- Project: `/Users/apple/CodexProjects/astra-os`
- Branch: `feature/p2-ai-intent-center`
- Version: `0.2.0-alpha.1`
- P1 baseline tag: `astra-os-p1-v0.1.0-alpha.1`
- P3 entered: no

P2 provides the independent Intent Service, Unix Socket JSON-RPC, capability-protected development HTTP and real WebSocket upgrade, configuration-driven rules, deterministic local model, candidate fusion, slot extraction, confidence/ambiguity, confirmation, safe cache, metrics, audit, Shell client, analysis UI, and minimum offline fallback.

## Measured Evidence

- Supported intents: 30
- Materialized examples: 290 Chinese, 87 English, 87 ambiguous, 87 negative
- P2 protocol schemas: 5
- P2 configuration schemas: 4
- CTest: 14 passed
- QML: 14 passed
- Python unit/contract/integration/security: 62 passed
- Performance/stability: 2 passed
- Live E2E: 1 passed
- Stability requests: 10,000, zero failures
- Stability audit: 40,020 valid records, zero Schema errors
- Audit contract: strict top-level service, session, intent, confidence, policy, duration, result, error, and details fields
- Metal: confirmed independently for both windows
- ARM64 Shell: confirmed

`make p2-run` created durable project processes through the macOS host launcher. The separate service and Shell PIDs remained alive after the command returned; health, readiness, metrics, Socket, two Metal windows, and 10 live intent scenarios were verified. `make p2-stop` removed only the two project launcher labels, PID/token files, and Socket, leaving no residual process.

The desktop accessibility tool could not attach to the unbundled Qt executable, so no screenshot is claimed. Visible-window creation, titles, controls, confirmation behavior, and projection state changes are covered by Qt/QML interaction tests and actual Metal first-frame logs.

## Result

Failure items: none. Warning items: screenshot automation unavailable for the unbundled development executable. Known limits: deterministic local classifier only, bounded injection signals, development one-frame WebSocket event snapshot, no cloud model, no P3 spatial input, and no real projector hardware.

Verification result: `P2_VERIFICATION_PASSED_WITH_SCREENSHOT_WARNING`.
