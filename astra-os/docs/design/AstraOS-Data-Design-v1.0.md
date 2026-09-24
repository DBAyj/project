# AstraOS Data Design v1.0

## Logical Entities and Relationships

`User` grants `Capability`; an `Intent` creates a `Task` with ordered `TaskStep` records; each privileged transition creates `AuditEvent`. `ProjectionSession` references `DisplayTarget`, `SpatialScene`, and privacy-classified `SceneObject`. `ModelDescriptor` describes `AIModel`; `MemoryRecord` is scoped to user or task; `ConfigurationRecord` and `SystemVersion` govern compatibility; `UpdatePackage` targets a version transition. Field-level baseline is [12 Logical Data Model](../architecture/12-data-model.md).

## Ownership, Lifecycle, and Storage

`astra-security` owns users and capabilities; task engine owns task/step state; spatial owns scene/anchor/object state; projection owns session state; model router owns descriptors; memory owns memory records; audit owns immutable events; settings owns configuration records; update owns package/version state. Target persistence is `/var/lib/astra-os`; logs are `/var/log/astra-os`; short-lived sockets/PIDs are `/run/astra-os`; caches are `/var/cache/astra-os`. macOS locations are adapter-provided and never core constants.

## Classification, Retention, Backup, Synchronization, Deletion, and Migration

High/critical records use encrypted secure stores and have no ordinary log payload. Caches are reconstructible and excluded from backup. Audit retention follows operations policy and preserves integrity hashes. Memory deletion removes primary content, derived indexes, and queued sync copies under a recorded deletion operation. Synchronization is not part of MVP; any future sync requires a policy-classified protocol. Schema versions are stored with persistent records; migrations are explicit, reversible where possible, validated before promotion, and included in update compatibility checks.
