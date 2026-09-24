# 10 Security Architecture

AstraOS uses capability-based authorization. Every action is evaluated as principal, capability, action, resource, object scope, time bound, and use bound; absence of an explicit grant is deny.

Core capabilities are `display.read`, `display.write`, `projection.start`, `projection.stop`, `projection.public`, `projection.private`, `projection.render`, `projection.control`, `camera.read`, `microphone.read`, `file.read`, `file.write`, `ai.invoke.local`, `ai.invoke.cloud`, `memory.read`, `memory.write`, `task.read`, `event.subscribe`, `settings.read`, `settings.write`, `update.install`, `audit.read`, `system.health.read`, and `system.shutdown`. The supervisor alone receives `system.shutdown`; ordinary modules receive neither capability by default.

`astra-security` authenticates and validates tokens; `astra-policy` makes contextual privacy and data-egress decisions. High-risk actions require secondary confirmation. An AI task is only a caller and cannot bypass these services. Every denial, cloud egress decision, projection decision, and elevated grant is audited. See [security boundary diagram](diagrams/astra-security-boundary.mmd) and [privacy model](11-privacy-model.md).
