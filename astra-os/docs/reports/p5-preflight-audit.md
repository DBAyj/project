# P5 Preflight Audit

Status: `P5_RELEASE_BASELINE_READY`

Required baseline marker: `P4_RELEASE_BASELINE_FINAL`.

## Repository State

- Project: AstraOS
- Branch: `feature/p5-spatial-ui-framework`
- Formal baseline: `astra-os-p4-v0.4.0-alpha.1` (`f414b8f`)
- P5 baseline merge: `db01c15`
- Repository version: `0.5.0-alpha.1`
- Version action: upgraded after the continuous P1-P4 tag ancestry passed
- Host: macOS on Apple Silicon (`arm64`)
- Qt: `6.11.1`
- Available project volume space at preflight: 161 GiB

## Prerequisite Status

| Phase | Actual status |
| --- | --- |
| P1 | `P1_RELEASE_GATE_PASSED`; tagged `astra-os-p1-v0.1.0-alpha.1` |
| P2 | `make p2-verify` passed with unit, contract, integration, security, performance, 10,000-request stability, and live service evidence; tagged `astra-os-p2-v0.2.0-alpha.1` |
| P3 | `make p3-verify` passed with protocol, image-pipeline, stability, camera-adapter, ProjectionTarget, and run/stop evidence; tagged `astra-os-p3-v0.3.0-alpha.1` |
| P4 | `P4_RELEASE_GATE_PASSED`, `P4_METAL_CONFIRMED`, visual/performance/100-cycle stability evidence passed; tagged `astra-os-p4-v0.4.0-alpha.1` |

P5 retains P4's deterministic fixture interfaces for repeatable regression and
also requires the independent P3-P4-P5 ProjectionTarget service chain for
release acceptance.

## P4 Interface Gate

| Required seam | Evidence | Result |
| --- | --- | --- |
| Projection runtime | `astra::projection::ProjectionRuntime` and strict render request/result | PASS |
| Projection output | `astra::render::ProjectionOutput` lifecycle and frame output | PASS |
| Projection layers | `astra::render::ProjectionLayer` and deterministic composition order | PASS |
| Privacy mask | final `PrivacyMaskPass` before output | PASS |
| Safe clear | `SafeClearController` covers privacy, target loss, render/output/service faults | PASS |
| Render state | explicit `ProjectionSession` states and service result state | PASS |
| Metal evidence | Phone and Projection windows reported actual Qt RHI Metal | PASS |

## Existing UI Baseline

- QML is split into Phone, Projection, shared components, and theme modules.
- `AstraTheme.qml` provides the current theme seam.
- Mouse drag, wheel zoom, buttons, full-screen controls, and simulated intent
  input exist in the P1 Shell.
- Core controls already expose Qt accessibility names and roles; P5 must extend
  this to every new interactive component and deterministic keyboard focus.
- P4 fixture output reaches the visible Projection window only through the
  capability-protected Projection Service client and P4 runtime.

## Gate Decision

The technical P4 seams are callable and fail closed, the predecessor release
chain is continuous, and the P3-P4-P5 service-chain acceptance test is present.
Every P5 report carries `P4_RELEASE_BASELINE_FINAL`. The cross-service evidence
proves a simulated/image-service ProjectionTarget boundary only; P5 does not
claim physical-space or projector-hardware validation and must not enter P6.
