# Phase P5 Spatial UI Requirements

All P5 evidence carries `P4_RELEASE_BASELINE_FINAL`. P5 uses deterministic P4
fixtures for repeatable regression and a live independent P3-P4-P5 service
chain for simulated/image-service ProjectionTarget acceptance. This does not
claim physical spatial-target or projector-hardware validation.

| ID | Requirement | Acceptance seam |
| --- | --- | --- |
| P5-COMP-001 | Components have typed identity, hierarchy, bounds, transform, privacy, accessibility, and audited lifecycle. | `SpatialUIComponent` |
| P5-WINDOW-001 | Windows support bounded create, show, hide, move, resize, target switching, save, and restore. | `SpatialWindowManager` |
| P5-LAYOUT-001 | Stack, grid, radial, freeform, and anchor-relative layouts preserve safe areas and protected overlays. | `SpatialLayoutEngine` |
| P5-FOCUS-001 | One focus owner exists per scope; system safety focus may preempt and destroyed/hidden owners release focus. | `SpatialFocusManager` |
| P5-INPUT-001 | Mouse, keyboard, touch, simulated gestures, system, and AI input use one router and deterministic hit testing. | `InputRouter` |
| P5-INTERACTION-001 | Selection, drag, resize, rotate, scale, cancel, rollback, and persistence use explicit state transitions. | `InteractionStateMachine` |
| P5-TASK-001 | P2-style fixture results create task surfaces without executing AI inference. | task fixture adapter |
| P5-NOTIFY-001 | Notifications obey severity, timeout, focus, privacy, and action rules. | `NotificationSurfaceManager` |
| P5-PRIVACY-001 | Every component is filtered before P4 submission; P4 remains the final privacy and safe-clear layer. | `PrivacyAwareUIService` |
| P5-STATE-001 | Only schema-versioned, non-sensitive UI state is persisted atomically. | `UIStateStore` |
| P5-ANIM-001 | Deterministic cancellable transitions support reduce-motion and immediate privacy hiding. | `UIAnimationCoordinator` |
| P5-A11Y-001 | Every interactive component exposes semantic role, name, state, value, actions, and keyboard focus. | accessibility service/QML |
| P5-SERVICE-001 | An independent capability-protected local service owns runtime state and rejects malformed requests. | Unix socket JSON-RPC |
| P5-SHELL-001 | Phone and Projection workspaces use one component model while preserving P1 controls and fallback. | Shell integration |
| P5-P4-001 | Visible projection components map only to public P4 `ProjectionLayer` values. | layer mapper |
| P5-CHAIN-001 | P5 accepts only a selected calibrated P3 service ProjectionTarget, preserves its UUID through P4 output, and clears stale output on target loss. | three-service target-chain acceptance |
| P5-TEST-001 | Unit, contract, QML, integration, accessibility, graphics, security, performance, and stability gates produce real evidence. | `make p5-release-gate` |

P5 excludes real gesture vision, eye tracking, physical projector/camera
drivers, multi-user spatial state, app/SDK ecosystems, AI avatars, P6 runtime,
and any direct manipulation of P4 warp, color, or offscreen internals.
