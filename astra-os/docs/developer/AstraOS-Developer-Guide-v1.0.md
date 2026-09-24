# AstraOS Developer Guide v1.0

## Project and Environment

AstraOS targets ARM64 Linux; macOS Apple Silicon is the development host. The environment audit reports are under [reports](../reports/). Portable code avoids host frameworks and hardcoded host paths. Repository ownership is defined in [Directory Layout](../architecture/06-directory-layout.md).

## Build, Test, and Module Work

The P0/P0.5 repository has no product executable. Run `make docs-verify` to validate the baseline. Future CMake/Ninja builds, Qt UI work, Rust services, and Python AI prototypes must follow the language responsibilities in [Development Convention](../architecture/14-development-convention.md). A new module starts with an ADR if it changes frozen boundaries, then schemas, interfaces, health check, contract tests, logs, audit classification, and implementation.

## Protocols, UI, Skills, Debugging, and Errors

Develop JSON-RPC and event contracts in `protocols` and `schemas` before clients. Build QML presentation around view models, keeping business transitions in services. AI Skills follow [Plugin Development](../sdk/AstraOS-Plugin-Development-v1.0.md). Debug with structured JSON logs, trace IDs, request IDs, and audit events; return registered error codes with redacted context.

## Submission and Phase Acceptance

Use the branch and commit rules in [Git Convention](../architecture/15-git-convention.md). Run relevant tests before review and provide actual evidence. P0/P0.5 ends here; P1 is the dual-window system simulator and requires a new instruction before code begins.
