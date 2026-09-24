# ADR-0015: Use Fixture Frame Relay For P4 Shell Preview

## Status

Accepted for P4 fixture-only validation.

## Context

`astra-projection-service` owns the P4 offscreen output, while `astra-shell`
owns the visible Qt RHI projection window. P3 real-world target evidence is not
available, so P4 must not introduce a shared-memory projector protocol or claim
a live hardware output path.

## Decision

The local, capability-protected `projection.output.frame` JSON-RPC method
returns the latest PUBLIC fixture frame as bounded PNG base64. The Shell writes
that frame to its project-local cache and displays it as a Qt Quick image. A
safe clear makes the method fail and removes the cache before the Shell can
render another frame.

The method is for deterministic P4 validation only. It is not a production
high-frequency graphics transport and carries
`P3_REAL_SPATIAL_INTEGRATION_NOT_VERIFIED` in every successful result.

## Consequences

The P4 evidence chain joins actual service output to the visible Metal/RHI
projection window while retaining a portable C++ renderer. A future production
frame transport requires a new ADR and a separate real-space P3 gate.
