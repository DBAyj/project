# 01 System Context

## Product Position

AstraOS is a mobile spatial operating system in which AI is the task-execution core and real space is an interaction surface. It coordinates a private phone display with a public projector display; it is neither an Android derivative nor a desktop application.

The first-generation validation loop is: user expresses a task, AI structures the intent, policy-approved system services execute it, private content remains on the phone, permitted public content appears on a projection surface, spatial input controls the scene, and audit records cover the whole operation.

## Actors and Boundaries

- **Local user** supplies voice, text, touch, gesture, eye, or camera input.
- **AstraOS device** owns local UI, policy, services, state, and audit data.
- **Projection and sensing devices** are untrusted external-device boundaries reached through the HAL.
- **Cloud model provider** is an optional external boundary. Only classified, policy-approved requests may leave the device.
- **Application and skill authors** consume future SDK APIs; they never receive unrestricted service access.

First-generation scope includes task-oriented UI, dual display, projection, 3D scenes, one-user tracking, gesture input, privacy classes, model routing, and ARM64 Linux. Air holograms, multi-viewer free-viewpoint holograms, custom silicon, cellular baseband, custom kernel, carrier certification, mass-market phones, and full Android compatibility are excluded.

See [context diagram](diagrams/astra-system-context.mmd) and [PRD](../product/AstraOS-PRD-v1.0.md).
