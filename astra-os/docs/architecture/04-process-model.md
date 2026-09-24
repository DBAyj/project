# 04 Process Model

## First-Generation Processes

| Process | Owned modules | Health and failure rule |
| --- | --- | --- |
| `astra-shell` | `astra-shell` | A UI crash must not terminate core services; supervisor may restart it. |
| `astra-core-service` | task, display, input, file, device, media, notification, settings, runtime | Publishes health; graceful shutdown drains accepted requests. |
| `astra-ai-service` | ai-runtime, intent, model-router, memory | AI failure leaves display and settings available; in-flight tasks become recoverable. |
| `astra-projection-service` | projection | Projection-service failure immediately closes external output and emits audit evidence. |
| `astra-security-service` | security, policy | Separate privileged service; callers cannot bypass its decision. |
| `astra-audit-service` | audit | Short outage writes to bounded local buffer, then flushes in order. |
| `astra-update-service` | update | Never modifies files used by an active process; coordinates verified reboot transition. |

Every process exposes `system.health` and `system.shutdown` over its local Unix socket. The supervisor owns liveness and restart policy; no module becomes a single super-process. See [process diagram](diagrams/astra-process-model.mmd).
