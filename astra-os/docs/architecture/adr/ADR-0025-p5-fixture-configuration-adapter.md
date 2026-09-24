# ADR-0025: Keep P5 Fixture Configuration Behind a Temporary Adapter

- Status: Accepted
- Date: 2026-07-15

## Context

The frozen configuration standard assigns YAML publication and validation to
`astra-settings`, with canonical files under `config/` and schemas under
`schemas/config/`. P5's deterministic fixture service needs five spatial UI
documents so that its isolated tests can exercise layout, input, accessibility,
and theme fail-closed behavior before the settings service is present in this
baseline.

## Decision

Retain the P5-only `spatial-*.yaml` fixture documents and their local schema
adapter inside `astra-spatial-ui-service`. They are explicitly fixture-mode
inputs, never the system configuration API. The adapter must validate every
document before constructing runtime options, use `SAFE_DEFAULTS` and disable
external projection on any failure, and expose this ownership in reports.

When `astra-settings` is available, the adapter will consume its immutable
validated snapshot and the fixture files will be removed without changing the
runtime interface.

## Consequences

P5 reports continue to carry `P4_RELEASE_BASELINE_FINAL` and must identify
the fixture-mode configuration boundary. No production service may depend on
these filenames or bypass `astra-settings`.
