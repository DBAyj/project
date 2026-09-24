# ADR-0010: Define Explicit Spatial Coordinate Systems

- Status: Accepted
- Date: 2026-07-15

## Context
Implicit pixel, view, surface, and world coordinates create unsafe and visually incorrect mappings.

## Decision
Every spatial point and transform declares one of the six P3 coordinate systems and every conversion declares source and target.

## Reasons And Alternatives
Typed names make protocol validation and migration reliable. Untyped vectors and platform-native coordinate objects were rejected.

## Impact, Risks, And Rollback
Payloads are larger and conversions explicit. Unsupported conversions fail with registered errors; rollback disables spatial mapping.
