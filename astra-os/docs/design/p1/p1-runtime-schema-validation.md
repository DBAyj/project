# P1 Runtime Configuration Schema Validation

`ConfigurationService` loads the P1 YAML file and validates it against `schemas/application-config.schema.json` before publishing a `SimulatorConfig`. The service checks the Schema document identity and required configuration groups, rejects invalid version or locale formats, and rejects unknown or malformed fields during parse. It does not depend on a Python pre-start validation step.

Valid configuration publishes `VALID`. Invalid Schema or content publishes `INVALID_FALLBACK_ACTIVE`; unavailable configuration publishes `LOAD_FAILED_FALLBACK_ACTIVE`. Both error paths select built-in safe defaults, record a configuration audit result, and publish warning text through `SystemStateModel`.

The Phone Display's system status panel renders a visible warning when `configurationWarningCount` is nonzero. CTest covers valid, missing Schema, unknown field, invalid version, and missing configuration paths. QML coverage verifies the Phone warning is visible for fallback state.
