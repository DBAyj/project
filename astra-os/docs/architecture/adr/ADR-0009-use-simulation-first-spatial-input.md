# ADR-0009: Use Simulation-First Spatial Input

- Status: Accepted
- Date: 2026-07-15

## Context
Camera hardware and authorization are nondeterministic in CI and remote development sessions.

## Decision
Default P3 to deterministic simulation, require image fixtures in CI, and treat camera as an optional measured adapter with automatic simulation fallback.

## Reasons And Alternatives
This keeps spatial behavior testable without claiming camera evidence. Camera-only acceptance and prerecorded third-party media were rejected.

## Impact, Risks, And Rollback
Simulation may not reflect all optics; reports separate simulated, image, video, and real-camera evidence. Rollback restores P2.
