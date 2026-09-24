# ADR-0016: Use An Independent Spatial UI Runtime

- Status: Accepted for P5 fixture-mode development
- Date: 2026-07-15

P5 uses an independent `astra-spatial-ui-service` with portable `astra-ui`
domain logic. The Shell is a client/view host and P4 remains the only projection
renderer. This isolates lifecycle and safety state from QML and permits service
restart with the P1/P4 fallback intact.
