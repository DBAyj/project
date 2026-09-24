# ADR-0013: Reallocate Spatial Error Codes For P3

- Status: Accepted
- Date: 2026-07-15

## Context

The P0 registry reserved `3001` as `SPATIAL_ANCHOR_NOT_FOUND`, but no implementation consumed it. The P3 specification assigns the 3001-3504 range by subsystem and requires 3001 for a service-not-started error and 3303 for a missing anchor.

## Decision

Before the first spatial implementation ships, move the unused anchor symbol to 3303 and register the complete P3 range exactly once in YAML and the C++ central registry.

## Reasons And Alternatives

Aligning the unshipped reservation with the reviewed P3 contract prevents duplicate codes. Keeping 3001 for anchors or creating a second private registry were rejected.

## Impact, Risks, And Rollback

No released caller changes because the old code was unused. Contract tests lock symbol uniqueness. Rollback removes the P3 range and restores the pre-P3 registry with P2 unaffected.
