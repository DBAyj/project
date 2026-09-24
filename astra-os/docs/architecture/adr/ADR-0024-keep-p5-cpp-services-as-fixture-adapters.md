# ADR-0024: Keep P5 C++ Services As Fixture Adapters

- Status: Accepted for P5 fixture-mode development
- Date: 2026-07-15

## Context

The frozen architecture assigns privileged policy evaluation and production
service state machines to isolated Rust services. P4 and P5 currently need
real local process, protocol, privacy, graphics, performance, and stability
evidence before the P2/P3/P4 release chain and production service supervisor
are available.

## Decision

The C++ `astra-projection-service` and `astra-spatial-ui-service` remain
deterministic fixture adapters for Phase P5. Portable C++ domain libraries may
implement UI, rendering, and test-fixture behavior, but they do not replace the
frozen Rust ownership of production security services or privileged lifecycle
state machines.

P5 fixture adapters must fail closed, use authenticated local transport, ask
`astra-policy` for every projection decision, and retain P4's final privacy
mask. Their public schemas are migration contracts for the future Rust service
implementations. All evidence continues to carry
`P4_RELEASE_BASELINE_FINAL`.

## Consequences

- P5 can exercise real independent processes without claiming a production
  security-service implementation.
- The Shell and QML remain clients and cannot self-authorize projection.
- A production Rust replacement must pass the same contracts and regression
  gates before this fixture exception can be retired.
- The predecessor release chain is now formal; P5 merge and tag eligibility is
  controlled by its own release gate.
