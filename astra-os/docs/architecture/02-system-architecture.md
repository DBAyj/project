# 02 System Architecture

## Frozen Layering

The architecture is layered to stop UI, AI, hardware, and security concerns from bypassing one another.

1. **User interaction** accepts voice, text, touch, gesture, eye, and camera signals.
2. **Spatial interface** presents Phone UI, Projection UI, and Spatial Scene through `astra-shell`, `astra-display`, and `astra-spatial`.
3. **AI runtime** parses intent, plans tasks, routes models, and manages memory.
4. **System services** execute file, device, media, notification, projection, audit, settings, and update responsibilities.
5. **Security capability** evaluates permission, privacy, and policy before a privileged operation.
6. **Application runtime** hosts Native, Web, WASM, and AI Skill workloads through constrained APIs.
7. **HAL** abstracts display, camera, sensor, AI acceleration, and projector devices.
8. **Kernel and drivers** are Linux-specific target-platform facilities.

## Boundary Rules

- Control flow moves from input or UI to intent, task engine, policy/security, and then a service.
- Data flows from services to the UI only through explicitly classified response or event schemas.
- Security is a mandatory decision boundary: no UI, AI, runtime, or HAL caller self-authorizes.
- `astra-*` services are separate processes; applications and external devices are separate trust boundaries.
- Cloud is an explicit egress boundary controlled by `astra-model-router` and `astra-policy`.

The formal diagram is [layered architecture](diagrams/astra-layered-architecture.mmd). Process detail is in [04 Process Model](04-process-model.md).
