# ADR-0023: Use Platform Paths And Bound Fixture Services

- Status: Accepted for P5 fixture-mode development
- Date: 2026-07-15

## Context

P5 runs on macOS during development but targets ARM64 Linux LTS. Runtime paths
cannot be embedded in core services, and the C++ P4/P5 service binaries used by
tests must not be represented as completed hardware or production platform
adapters.

## Decision

`astra::common::PlatformPaths` owns host-specific defaults for sockets, state,
audit, and development project roots. Core UI, projection, protocol, and policy
types receive resolved paths through constructors or command-line options and
do not depend on the current working directory or `/Users/...` paths.

The C++ `astra-projection-service` and `astra-spatial-ui-service` processes are
deterministic fixture/runtime boundaries for protocol, privacy, safe-clear,
Shell integration, performance, and stability evidence. They do not claim P3
sensor integration, physical projector control, device drivers, or a frozen
production service manager. After the predecessor release chain became formal,
every P5 report records `P4_RELEASE_BASELINE_FINAL`.

## Consequences

- macOS development and Linux target paths can evolve in one adapter.
- Tests may start real independent local processes without binding domain code
  to the development host.
- Fixture evidence proves repeatable P4/P5 contracts and safety behavior. The
  separate three-service acceptance proves only the P3 process/protocol target
  boundary; neither is physical projection evidence.
- Replacing fixture processes with production adapters does not change the
  portable domain or JSON-RPC interfaces.
