# ADR-0003: Use Capability-Based Security

- Status: Accepted
- Date: 2026-07-14

## Context

AI-driven task execution and external projection need precise, auditable authority boundaries.

## Decision

Authorize system operations with expiring, object-scoped, use-limited capabilities evaluated by a separate security and policy service.

## Reasons and Alternatives

Capabilities express least privilege more directly than broad role-only grants. UI-owned privacy decisions and AI self-authorization are rejected because they permit bypass paths.

## Impact, Risks, and Rollback

Token issuance and revocation need resilient storage and clock handling. Service APIs retain explicit action/resource inputs, so a future authorization backend can replace token representation without changing callers.
