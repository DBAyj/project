# ADR-0026: Bind P5 Projection To A P3 Service Target

- Status: Accepted for P5 release remediation
- Date: 2026-07-16

## Context

P4 originally accepted only deterministic geometry fixtures and required
`P3_REAL_SPATIAL_INTEGRATION_NOT_VERIFIED`. That constraint prevented a P5
release gate from proving that an independently running P3 service produced the
projection target used by the P4/P5 service chain.

The available P3 service can produce a schema-validated `ProjectionTarget`
from simulation or image input. This is real process and protocol evidence, but
it is not physical projector, camera, or surface evidence.

## Decision

P5 may configure its P4 gateway with a capability-protected P3 Unix socket. The
gateway reads `spatial.state` immediately before submitting visible P5 layers.
It accepts only a `TRACKING` state whose target is selected, has a UUID target
and surface identity, has four finite `IMAGE_PIXEL` corners, and is
`CALIBRATED` or `ACTIVE` with a usable quality.

P4 accepts that unchanged target envelope with
`P3_SERVICE_PROJECTION_TARGET_VERIFIED`, validates it again, and records the
marker and target ID on the output frame. Missing, stale, malformed, degraded,
lost, or unverified targets fail closed and clear output. Fixture-only P4
requests remain available for deterministic P4 regression, but they cannot
satisfy the P5 cross-service release gate.

## Consequences

- The P5 acceptance test must start independent P3, P4, and P5 processes and
  prove that the P3 target ID reaches the P4 output through a P5 layer submit.
- P3 target loss must prevent subsequent P5 projection submission and leave no
  stale P4 frame.
- Capability tokens remain process-local inputs and are never copied into the
  target envelope, state file, output frame, or report.
- This decision proves the simulated/image service integration boundary only.
  It does not claim physical spatial-target validation or projector hardware.
