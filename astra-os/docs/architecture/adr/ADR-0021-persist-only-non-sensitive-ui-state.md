# ADR-0021: Persist Only Non-Sensitive UI State

- Status: Accepted for P5 fixture-mode development
- Date: 2026-07-15

P5 persists strict versioned layout preferences only. Credentials, capability
tokens, confirmation tokens, sensitive task text, and unredacted AI input are
prohibited. Invalid state is rejected and a safe layout is restored.
