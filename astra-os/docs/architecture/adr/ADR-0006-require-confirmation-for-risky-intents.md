# ADR-0006: Require Confirmation for Risky Intents

- Status: Accepted
- Date: 2026-07-14

## Context

Privacy expansion, projection clearing, and uncertain actions can expose or remove content even when intent classification is plausible.

## Decision

Use expiring, request-bound, single-use confirmation records for risky or mid-confidence intents; ambiguity asks clarification and low confidence rejects.

## Reasons and Alternatives

Confidence alone does not represent action risk. Unconditional auto-execution and UI-only confirmation were rejected because both bypass service policy.

## Impact, Risks, and Rollback

Confirmation adds user friction and state expiry. Audit and replay tests are required; rollback may conservatively reject every risky action.
