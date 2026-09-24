# 14 Development Convention

| Language or format | Frozen responsibility |
| --- | --- |
| QML | Interface composition and interaction presentation. |
| C++ | Graphics, rendering, real-time interaction, and Qt integration. |
| Rust | Security-sensitive services, core state machines, and low-level services. |
| Python | AI prototypes, model orchestration, intent services, and developer tooling. |
| JSON Schema | Contract and configuration validation. |
| YAML | Versioned system configuration. |
| Shell | Build, install, test, and operations scripts only. |

QML must not contain complex business logic. Python must not perform high-frequency rendering. Shell is not business logic. C++ may not bypass policy/security, and protocols may not be reimplemented independently in multiple languages. New core languages need architecture review.

Use portable path abstractions, explicit UTF-8, stable UUID identifiers, structured error returns, correlation IDs, and tests at public interfaces. Format and lint configuration follows the future build toolchain; no host-specific framework imports may enter portable modules.
