# ADR-0007: Keep Safe Rule Fallback in Shell

- Status: Accepted
- Date: 2026-07-14

## Context

An independent intent process can be unavailable while projection content still needs an immediate safe-stop path.

## Decision

Keep only stop projection, hide projection content, and show system status as in-process Shell fallback rules. All expansive or unknown actions deny.

## Reasons and Alternatives

This preserves safety without recreating the P2 classifier in QML or Shell. No fallback and a complete duplicate engine were rejected.

## Impact, Risks, and Rollback

Shell contains a deliberately small rule set that must remain synchronized with safety tests. Rollback leaves these rules active and disables the service client.
