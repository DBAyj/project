# AstraOS PRD v1.0

## Product Background and Goal

Personal computing keeps private content on a phone while shared context often needs a larger physical surface. AstraOS tests a system where AI understands a user task, governs actions through policy, and coordinates a private screen with projection and spatial interaction. The goal is to validate this closed loop on a single-user ARM64 Linux terminal before attempting a consumer phone.

## Target Users and Scenarios

The initial user is a technically capable individual working alone in a room with a phone display, a projection surface, and a camera-capable terminal. Typical scenarios are: planning a task by voice or text; presenting a privacy-classified scene to a desk or wall; manipulating a 3D object by gesture; reviewing system action history; and declining a cloud route for sensitive content.

## Product Boundary and Core Value

Core value is controlled task completion across private, shared, and spatial surfaces with auditable safety decisions. The first generation includes task UI, dual display, desktop/wall projection, 3D display, one-user view tracking, camera gesture recognition, privacy levels, local/cloud routing, and ARM64 Linux. It excludes true air holograms, multi-user free-viewpoint holograms, custom chips/baseband/kernel, carrier work, production phone manufacturing, and full Android compatibility.

## User Journey

1. The user submits a task by text, voice, touch, or spatial input.
2. Intent parsing exposes confidence and ambiguity; the user clarifies when necessary.
3. The task engine proposes and executes policy-authorized steps.
4. Private detail appears only on Phone UI; permitted public material is routed to Projection UI.
5. Spatial input modifies permitted scene objects.
6. The audit service records requests, decisions, actions, denials, and outcomes.

## Functional Map and Priority

| Priority | Capability | MVP disposition |
| --- | --- | --- |
| Must | Intent, task state, capability enforcement, audit trail | Required for the validation loop. |
| Must | Phone/projection separation and privacy policy | Required for safe shared output. |
| Should | Spatial scene and one-user gesture control | Required for spatial-loop validation. |
| Should | Local/cloud model routing | Required for privacy-aware AI evaluation. |
| Later | Plugin/SDK ecosystem and advanced media | Designed now, implemented after platform core. |

## Non-Functional, Privacy, Security, and Performance Expectations

The product is portable to ARM64 Linux, remains usable if AI or projection fails, emits structured logs, has audit coverage for privileged actions, uses deny-by-default capabilities, and never projects `PRIVATE_SCREEN_ONLY` or `NO_PROJECTION` content. P1 will measure end-to-end task latency, display responsiveness, scene frame cadence, and recovery time against hardware-specific budgets; this baseline deliberately does not invent hardware performance numbers.

## Hardware Assumptions and Acceptance

The validation device has ARM64 Linux, private display, optional projector, camera/sensors, network access, and a HAL-supported graphics path. Acceptance requires an auditable task loop, correct privacy routing, isolated process failures, schema-validated contracts, and documented recovery behavior.

## Risks and Roadmap

The principal risks are projection brightness/power/thermal constraints, gesture reliability, single-viewer constraints, AI misoperation, privacy leakage, ecosystem bootstrapping, platform drift, hardware drivers, model size, networking, update failure, and supply chain dependency. See [risk register](../project/risk-register.md). The roadmap is E0 environment, P0 architecture, P0.5 design baseline, P1 dual-window simulator, P2 AI intent, P3 spatial, P4 projection, P5 services, P6 runtime/SDK, P7 ARM64 board, P8 projection/camera prototype, P9 portable terminal, and P10 manufacturing evaluation.
