# AstraOS Plugin Development v1.0

## Plugin Types and Manifest

Plugins may be UI extensions, AI Skills, data connectors, or device integrations. A signed manifest declares plugin ID, version, compatible SDK range, type, entry point, requested capabilities, data classification, and lifecycle hooks. Unknown manifest fields are rejected by the strict [plugin manifest schema](../../schemas/plugins/plugin-manifest.schema.json).

## Sandbox, Lifecycle, and Audit

Plugins run through `astra-runtime` sandbox boundaries. Lifecycle is discovered, verified, installed, enabled, suspended, disabled, upgraded, removed. A plugin cannot access services, projection, camera, memory, or cloud APIs without runtime-mediated capability and policy checks. Installation, rejection, grant/denial, update, and removal are audited.

## Version, Installation, Uninstallation, Upgrade, and Signing

The runtime verifies publisher signature, compatibility, and manifest before installation. Upgrade uses side-by-side validation and state migration only after a compatible version check. Uninstall revokes capabilities and executes declared data-retention policy. A failed upgrade keeps the earlier verified plugin active.
