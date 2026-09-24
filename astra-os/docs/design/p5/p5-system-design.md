# P5 Spatial UI System Design

`astra-ui` owns portable domain behavior. `astra-spatial-ui-service` owns the
component tree, window/layout/focus/input state, persistence, audit, and P4
layer mapping. The Shell is a client and view host. Production control uses
capability-protected JSON-RPC over a Unix socket; an explicitly configured
development adapter may expose diagnostics on a non-default public address.

The fixed path is input -> router -> focus/interaction -> component tree ->
privacy filter -> P4 layer mapper -> Projection Runtime. No QML code bypasses
the service or P4. `P4_RELEASE_BASELINE_FINAL` is retained in every report.
Before visible layers are submitted, the P5 gateway queries the authenticated
P3 service and validates a selected calibrated ProjectionTarget. P4 validates
the same envelope again, records its UUID on the output frame, and clears stale
content when P3 reports target loss.

Startup loads `spatial-ui.yaml`, `spatial-layout.yaml`, `spatial-input.yaml`,
`spatial-accessibility.yaml`, and `spatial-theme.yaml` and validates each against
its draft 2020-12 Schema. Missing or invalid input selects `SAFE_DEFAULTS`,
disables the external projection target, and exposes the warning in service
status. Valid values configure runtime limits, persistence, layout, input, and
accessibility behavior.

ADR-0024 limits the current C++ P4/P5 services to deterministic fixture
adapters. The frozen production architecture keeps privileged policy and
service state-machine ownership in isolated Rust services; replacement
implementations must retain the same public schemas and release gates.
