# AstraOS P4 Preflight Audit

## Execution Context

- Executed: 2026-07-15 (Asia/Shanghai)
- Branch: `feature/p4-projection-rendering-engine`
- Baseline: `develop` at `49b819e`
- Version: `0.3.0-alpha.1`
- P4 mode: deterministic spatial fixtures

## P1 Status

P1 is accepted. `docs/reports/p1-release-gate-report.md` records
`P1_RELEASE_GATE_PASSED`, including real Qt RHI/Metal window creation,
first-frame, dual-window interaction, audit, policy, and accessibility evidence.
The existing Shell safety services remain the mandatory offline fallback.

## P2 Status

P2 is functionally verified on `feature/p2-ai-intent-center`, but is not
integrated into `develop`. The P3 verification run executed `verify_p2.sh`
successfully. P4 may expose a P2 adapter contract and test double only; it must
not claim a production intent-to-projection integration on this baseline.

## P3 Status

P3 is functionally verified on `feature/p3-spatial-scene-engine`, but is not
integrated into `develop`. Its current evidence includes 9 CTest targets, 6
contract tests, a service run/stop cycle, live E2E, and 10,000 stability frames.
The measured pipeline P95 is 1.593792 ms, throughput is 800 FPS, and event
latency P95 is 2.407292 ms.

`P3_REAL_SPATIAL_INTEGRATION_NOT_VERIFIED`

The evidence uses `SIMULATION` and `WORLD_SIMULATED`. It does not verify a
physical projection target, physical camera calibration, projector discovery,
or dynamically changing Homography output.

## Reusable Interfaces

- P1 `ProjectionSessionService`, `ProjectionPolicyService`, `AuditLogService`,
  error registry, trace context, and the two-window Qt Shell.
- P3 public contracts, when later available through JSON-RPC:
  `ProjectionTarget`, `SpatialScene`, `SceneObject`, `CalibrationProfile`,
  `SpatialState`, and Homography data.
- `protocols/projection/projection-session-v1.schema.json` is retained as the
  P1 compatibility contract. P4 adds versioned contracts rather than changing
  its existing state enumeration.

## Missing Interfaces

- `ProjectionRenderRequest`, P4 `ProjectionOutput`, P4 session lifecycle,
  layer ordering, render graph, warp, color compensation, privacy-mask, and
  safe-clear contracts.
- An independent projection service, portable render library, fixtures,
  verification commands, and P4 graphics evidence.
- A public P3-to-P4 target-expiry and output-calibration adapter contract.

## Entry Decision

`P4_FULL_SPATIAL_INTEGRATION_PRECONDITIONS_NOT_MET`

P4 may proceed only with the deterministic fixtures `front-rectangle`,
`left-keystone`, `right-keystone`, `top-keystone`, `bottom-keystone`,
`invalid-self-intersection`, and `invalid-non-invertible`. Every P4 report will
retain `P3_REAL_SPATIAL_INTEGRATION_NOT_VERIFIED`. No result may claim actual
projector output, physical camera calibration, target discovery, or dynamic
Homography validation.
