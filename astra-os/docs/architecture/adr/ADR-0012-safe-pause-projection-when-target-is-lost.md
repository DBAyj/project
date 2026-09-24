# ADR-0012: Safe-Pause Projection When Target Is Lost

- Status: Accepted
- Date: 2026-07-15

## Context
Continuing to render against stale geometry can expose content outside the approved surface.

## Decision
When the selected target becomes `LOST` or `INVALID`, P3 emits a safety event and Shell clears sensitive content before pausing or returning the P1 projection session to safe idle.

## Reasons And Alternatives
Fail-closed behavior preserves P1 privacy guarantees. Freezing the last frame or trusting stale geometry was rejected.

## Impact, Risks, And Rollback
Temporary vision loss interrupts projection and requires redetection. Rollback disables spatial targeting and returns to the fixed P1 window.
