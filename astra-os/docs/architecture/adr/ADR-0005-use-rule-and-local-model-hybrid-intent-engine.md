# ADR-0005: Use Rule and Local Model Hybrid Intent Engine

- Status: Accepted
- Date: 2026-07-14

## Context

Rules provide precision and safety while natural-language variation needs broader deterministic classification without a large model.

## Decision

Combine strict configuration-driven rules with a deterministic local similarity adapter and merge typed candidates using configured weights.

## Reasons and Alternatives

The hybrid remains offline, reproducible, explainable, and independently degradable. Cloud-only, large local models, and rules-only P2 were rejected.

## Impact, Risks, and Rollback

Candidate calibration can drift as examples grow. Metrics and regression corpora constrain changes; either engine can be disabled, and rules remain the final fallback.
