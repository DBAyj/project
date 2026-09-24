# ADR-0017: Use Centralized Input Routing

- Status: Accepted for P5 fixture-mode development
- Date: 2026-07-15

All platform input adapters emit one versioned event to `InputRouter`.
Components do not independently interpret platform events. This guarantees
protected-layer ordering, consistent audit, and deterministic simulation.
