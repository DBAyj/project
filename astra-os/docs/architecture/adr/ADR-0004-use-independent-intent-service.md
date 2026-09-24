# ADR-0004: Use Independent Intent Service

- Status: Accepted
- Date: 2026-07-14

## Context

P1 intent rules run inside the UI process, which prevents process isolation and a stable API for later phases.

## Decision

Run `astra-intent-service` as an independent Python 3.12 process using JSON-RPC over a Unix socket; explicit development mode may expose HTTP.

## Reasons and Alternatives

Isolation preserves the shell when classification fails and matches the frozen process/IPC model. Keeping all logic in Shell and adopting a message broker were rejected.

## Impact, Risks, and Rollback

IPC adds lifecycle and timeout handling. Shell retains a minimal safety fallback, and rollback disables the client and restores P1-only parsing.
