# P4 Projection System Design

## Boundaries

`astra-projection-service` owns the P4 session, projection render request,
render graph, output adapter, and safe-clear state. `astra-render` owns pure
render data, geometry, composition, and portable output interfaces. The Shell
uses a public local client and retains P1 controls as an independent fallback.
For deterministic P4 evidence, the capability-protected local
`projection.output.frame` method relays a PUBLIC fixture PNG to the visible Qt
projection window; safe clear invalidates both the service frame and Shell cache.
P2 and P3 are optional adapters at the JSON-schema boundary only.

## Fixed Render Graph

`InputPass -> ScenePass -> LayerCompositionPass -> CropAndOverscanPass ->
GeometryWarpPass -> ColorCompensationPass -> PrivacyMaskPass -> OutputPass`

Every pass declares inputs and outputs. The graph topologically sorts them and
rejects absent dependencies and cycles before rendering. An unavailable output
or a failed pass invokes safe clear. `PrivacyMaskPass` is final content
composition immediately before `OutputPass`; ordinary layers cannot run after it.

## Session Lifecycle

P4 states are `IDLE`, `INITIALIZING`, `READY`, `RENDERING`, `PAUSED`,
`CLEARING`, `STOPPING`, `STOPPED`, and `ERROR`. Transitions are explicit,
serialized, and audited. Clear failure yields `ERROR` and still delegates to the
P1 Shell hide/stop fallback. No code may retain or re-present the last frame
after a safety failure.

## Fixture-Only Spatial Input

The fixture adapter accepts only named, versioned deterministic profiles:
`front-rectangle`, four keystones, `invalid-self-intersection`, and
`invalid-non-invertible`. The request and all reports contain
`P3_REAL_SPATIAL_INTEGRATION_NOT_VERIFIED`. A later P3 adapter may pass a
public target/calibration version only after a separate real-space gate.
