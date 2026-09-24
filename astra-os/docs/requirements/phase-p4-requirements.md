# Phase P4 Projection Rendering Requirements

## Scope

P4 provides a portable projection render service with deterministic input
fixtures, offscreen composition, geometry correction, color compensation,
privacy masking, safe clear, scheduling, local JSON-RPC, and Shell manual
controls. P4 consumes P3 only through a future public adapter. The current
baseline is fixture-only and carries `P3_REAL_SPATIAL_INTEGRATION_NOT_VERIFIED`.

## Requirements

| ID | Requirement | Acceptance seam |
| --- | --- | --- |
| P4-PROTOCOL-001 | Public P4 requests, results, events, and errors use strict versioned schemas. | schema contracts |
| P4-OUTPUT-001 | An output supports initialize, present, clear, shutdown, state, resolution, and refresh rate. | `ProjectionOutput` |
| P4-SESSION-001 | P4 session accepts only documented lifecycle transitions and audits rejected transitions. | `ProjectionSession` |
| P4-LAYER-001 | Layers sort deterministically and `PRIVACY_MASK` is above ordinary content. | `ProjectionLayer` |
| P4-GRAPH-001 | Graph execution rejects missing dependencies, cycles, pass failure, and unavailable output. | `RenderGraph` |
| P4-WARP-001 | Valid convex quadrilaterals support deterministic Homography correction. | `Homography` |
| P4-WARP-002 | Self-intersection, small area, non-invertibility, non-finite values, and unsafe bounds fail closed. | geometry validator |
| P4-MESH-001 | A bounded mesh supports local warp and rejects folded cells. | `MeshWarp` |
| P4-COLOR-001 | Crop, overscan, gamma, and color compensation are bounded and deterministic. | render passes |
| P4-PRIVACY-001 | Privacy policy controls final-pass full, rectangle, polygon, and solid masks. | `PrivacyMask` |
| P4-SAFETY-001 | No projection, private-only policy, target loss, warp failure, pass failure, output loss, and service fault immediately clear output. | safe-clear integration |
| P4-SERVICE-001 | The projection service exposes only capability-protected local JSON-RPC. | service contract |
| P4-SHELL-001 | P1 dual-window controls, audit trace, privacy denial, and offline fallback remain operational. | Shell integration |
| P4-TEST-001 | Project-generated fixtures cover rectangle, keystone, invalid geometry, color, layer, and privacy scenarios. | visual tests |
| P4-GRAPHICS-001 | macOS evidence proves actual RHI/Metal, scene initialization, first frame, projection-output first frame, and visible window. | graphics verifier |
| P4-PERF-001 | Release evidence records full-pipeline P95, FPS, first-frame time, and safe-clear latency. | performance report |
| P4-STABILITY-001 | Required render and lifecycle cycles leave no active process, stale frame, illegal state, or runtime residue. | stability/run-stop tests |

## Exclusions

P4 does not implement SLAM, ARKit, OpenXR, LiDAR, depth drivers, gestures,
eye/face tracking, multi-user spatial maps, 3D reconstruction, NeRF, cloud
vision, physical projector discovery, physical camera calibration, or P5 work.
It does not claim verified physical-space integration while P3 is simulation-only.
