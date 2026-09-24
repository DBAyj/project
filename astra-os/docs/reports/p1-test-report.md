# P1 Test Report

- Executed: 2026-07-14 (Asia/Shanghai)
- Primary command: `make p1-release-gate`
- Result: PASS
- CTest: 11/11 passed
- Schema contract tests: 4/4 passed
- QML tests: 9/9 passed
- Graphics verifier unit tests: 4/4 passed
- Unique test-case total: 28/28 passed
- Failed: 0
- Skipped: 0
- Focused policy test: 1/1 passed
- Focused error-registry test: 1/1 passed
- Focused configuration-runtime test: 1/1 passed
- Fresh audit evidence: 920/920 JSONL records validated against the audit Schema
- Stability loop: 100 start/pause/resume/stop cycles passed
- Accessibility: QML project warning count 0; accessible metadata checks passed
- Graphics/window evidence: `METAL_CONFIRMED` and `WINDOW_INTERACTION_EVIDENCE_PASSED`

Covered public seams are common privacy/error/identifier types, configuration fallback, deterministic intent parsing, projection policy and state, JSONL audit writing, application bootstrap, controller synchronization, protocol/configuration schemas, and QML windows/components. A dedicated CTest uses the real controller and real window components for execution, drag rotation, wheel zoom, pause/resume, reset, fullscreen `QWindow` visibility, stop, and denial. The QML test uses pointer events to verify control semantics; it is paired with real QRhi first-frame output from both native windows.
