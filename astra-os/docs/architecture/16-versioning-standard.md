# 16 Versioning Standard

AstraOS uses semantic system versions: `MAJOR.MINOR.PATCH`, with pre-releases such as `0.1.0-alpha.1` and `0.2.0-beta.1`. The current version is read from [VERSION](../../VERSION).

System, protocol, configuration, database, SDK, and model-interface versions evolve independently. A compatibility handshake compares the relevant contract versions before an operation. Backward-compatible additions increase a minor or local contract revision; removals, changed semantics, or incompatible privacy/security behavior require a major contract transition and migration path.
