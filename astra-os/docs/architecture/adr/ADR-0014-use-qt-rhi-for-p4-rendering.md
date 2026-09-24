# ADR-0014: Use Qt RHI For P4 Rendering

- Status: Accepted
- Date: 2026-07-15

## Context

P4 needs real macOS Metal evidence while the product runtime targets ARM64 Linux
with Vulkan or OpenGL ES. Projection state, privacy, geometry validation, and
render-graph behavior must not depend on a host-only graphics API.

## Decision

Use a portable `astra-render` library with a Qt RHI-backed output adapter. The
renderer owns no Metal-specific business logic. `WindowProjectionOutput` is the
macOS development adapter and reports real Qt RHI/Metal scene initialization,
visible-window state, and first-frame evidence. Linux adapters select the
available Qt RHI backend through platform configuration.

## Reasons And Alternatives

Qt RHI keeps the renderer portable and permits the required Metal validation.
Direct Metal APIs are rejected because they violate the ARM64 Linux boundary.
CPU-only image generation is retained for deterministic unit/visual tests but
cannot replace graphics-runtime evidence. A P3 camera/projector runtime is out
of scope because P3 real spatial integration is not verified.

## Impact, Risks, And Rollback

Backend differences require visual and graphics tests on each supported
platform. All public values remain schema-defined and portable. If the P4
adapter fails, the Shell invokes P1 stop/hide projection behavior and P4 clears
its output; no stale frame may remain visible.
