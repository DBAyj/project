# ADR-0022: Support Simulation-First Gesture Input

- Status: Accepted for P5 fixture-mode development
- Date: 2026-07-15

P5 supports typed select, grab, release, scale, and rotate gesture events from a
deterministic simulator. Visual gesture recognition and camera dependencies are
out of scope. A future adapter must preserve the same routed event contract.
