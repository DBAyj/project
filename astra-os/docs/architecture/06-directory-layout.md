# 06 Directory Layout

The root directory is frozen. New root directories require an ADR.

| Directory | Responsibility |
| --- | --- |
| `apps` | User-facing UI applications. |
| `services` | Independent system-service executables. |
| `libraries` | Reusable cross-platform libraries. |
| `runtimes` | AI, application, and plugin runtimes. |
| `hal` | Hardware abstraction and platform adapters. |
| `protocols` | IPC and network registries/specifications. |
| `schemas` | JSON Schema for public contracts and configuration. |
| `config` | Default, sample, and versioned configuration. |
| `assets` | Icons, images, and small demonstration resources. |
| `models` | Model manifests only; large model binaries are ignored. |
| `plugins` | Approved first-party plugins and Skills. |
| `scripts` | Build, test, run, and verification tooling only. |
| `tests` | Cross-module and acceptance tests. |
| `tools` | Developer tools. |
| `docs` | Architecture, product, design, and reports. |
| `packaging` | Package and release definitions. |
| `runtime` | Uncommitted runtime data. |
| `third_party` | Explicitly approved third-party source/dependency records. |

Target-device paths are `/etc/astra-os`, `/var/lib/astra-os`, `/var/log/astra-os`, `/var/cache/astra-os`, `/run/astra-os`, `/opt/astra-os`, and `/usr/lib/astra-os`. macOS adapters may use `~/Library/Application Support/AstraOS`, `~/Library/Logs/AstraOS`, and `~/Library/Caches/AstraOS`; core code obtains them only through a platform-path adapter.
