# 07 Configuration Standard

Configuration uses YAML with JSON Schema validation. The frozen files are `astra.example.yaml`, `system.yaml`, `display.yaml`, `projection.yaml`, `spatial.yaml`, `ai.yaml`, `privacy.yaml`, `security.yaml`, `logging.yaml`, `update.yaml`, and `network.yaml`; each has a same-named schema under `schemas/config/`.

Precedence is built-in defaults, system configuration, device configuration, environment variables, then startup arguments. Unknown fields are errors. Secrets are never ordinary YAML values and are supplied through secure storage. A configuration change creates `ConfigurationRecord` and an audit event. `network.public_host` is blank until configured and must not default to `localhost` or `127.0.0.1`.

Configuration loading belongs to `astra-settings`, validates before publication, is immutable per request, and exposes a versioned snapshot. See [Data Design](../design/AstraOS-Data-Design-v1.0.md).
