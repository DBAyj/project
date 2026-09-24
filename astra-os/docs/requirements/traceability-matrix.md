# AstraOS Requirements Traceability Matrix

| Requirement ID | Requirement description | Module | Design document | Test type | Implementation phase |
| --- | --- | --- | --- | --- | --- |
| FR-INTENT-001 | Structured intent with confidence | astra-intent | [P2 Pipeline](../design/p2/p2-intent-pipeline.md) | unit, contract, integration | P2 |
| FR-INTENT-002 | Ambiguity clarification | astra-intent | [P2 Confidence](../design/p2/p2-confidence-and-ambiguity.md) | component, integration, E2E | P2 |
| FR-TASK-001 | Ordered task states | astra-task-engine | [LLD](../design/AstraOS-LLD-Baseline-v1.0.md) | state-machine, integration | P2 |
| FR-TASK-002 | Retry, cancellation, compensation | astra-task-engine | [LLD](../design/AstraOS-LLD-Baseline-v1.0.md) | unit, recovery | P2 |
| FR-PROJECTION-001 | Approved projection session | astra-projection | [Protocol](../protocols/AstraOS-Protocol-Specification-v1.0.md) | contract, hardware | P4 |
| FR-PROJECTION-002 | Private-output block | astra-policy | [Privacy](../architecture/11-privacy-model.md) | policy, security | P4 |
| FR-DISPLAY-001 | Private and external targets | astra-display | [HLD](../design/AstraOS-HLD-v1.0.md) | component, hardware | P1 |
| FR-SPATIAL-001 | Scene/anchor/object model | astra-spatial | [Data Design](../design/AstraOS-Data-Design-v1.0.md) | unit, hardware | P3 |
| FR-AI-001 | Policy-routed model invocation | astra-model-router | [P2 Model Adapter](../design/p2/p2-model-adapter.md) | contract, fallback, security | P2 |
| FR-MEMORY-001 | Isolated and deletable memory | astra-memory | [Data Design](../design/AstraOS-Data-Design-v1.0.md) | security, deletion | P2 |
| FR-SECURITY-001 | Capability-based denial | astra-security | [Security](../architecture/10-security-architecture.md) | allow/deny, adversarial | P4 |
| FR-SECURITY-002 | Grant and denial context | astra-security | [Protocol](../protocols/AstraOS-Protocol-Specification-v1.0.md) | integration, audit | P4 |
| FR-AUDIT-001 | Audit privileged activity | astra-audit | [Data Design](../design/AstraOS-Data-Design-v1.0.md) | integrity, recovery | P4 |
| FR-CONFIG-001 | Strict YAML Schema validation | astra-settings | [Configuration](../architecture/07-configuration-standard.md) | schema, negative | P5 |
| FR-UPDATE-001 | Verify package before staging | astra-update | [Update Design](../architecture/18-update-and-rollback-design.md) | signature, integration | P5 |
| FR-UPDATE-002 | Automatic rollback | astra-update | [Update Design](../architecture/18-update-and-rollback-design.md) | recovery, hardware | P5 |
| FR-RUNTIME-001 | Capability-mediated runtime | astra-runtime | [SDK Design](../sdk/AstraOS-SDK-Design-v1.0.md) | sandbox, E2E | P6 |
| NFR-PERF-001 | Measurable performance telemetry | astra-common | [HLD](../design/AstraOS-HLD-v1.0.md) | performance | P1 |
| NFR-SEC-001 | Deny-by-default capabilities | astra-security | [Security](../architecture/10-security-architecture.md) | adversarial | P4 |
| NFR-RELIABILITY-001 | Failure isolation and output closure | astra-projection | [Process Model](../architecture/04-process-model.md) | fault injection | P4 |
| NFR-PORTABILITY-001 | ARM64 Linux portable core | astra-hal | [Platform Abstraction](../architecture/19-platform-abstraction.md) | portability | P7 |
| NFR-MAINT-001 | Versioned contracts | astra-sdk | [Protocol](../protocols/AstraOS-Protocol-Specification-v1.0.md) | compatibility | P6 |
| NFR-TEST-001 | Health and acceptance tests | astra-common | [Testing Strategy](../architecture/17-testing-strategy.md) | integration | P1 |
| P5-COMP-001 | Typed, privacy-labeled spatial component lifecycle | astra-ui | [P5 Component Model](../design/p5/p5-component-model.md) | unit, lifecycle | P5 |
| P5-WINDOW-001 | Bounded multi-target spatial window management | astra-ui | [P5 Window Manager](../design/p5/p5-spatial-window-manager.md) | unit, interaction | P5 |
| P5-LAYOUT-001 | Safe stack, grid, radial, freeform, and anchor layouts | astra-ui | [P5 Layout Engine](../design/p5/p5-layout-engine.md) | unit, performance | P5 |
| P5-FOCUS-001 | Single deterministic focus owner per scope | astra-ui | [P5 Focus](../design/p5/p5-focus-management.md) | unit, accessibility, stability | P5 |
| P5-INPUT-001 | Central safety-aware spatial input routing | astra-ui | [P5 Input Routing](../design/p5/p5-input-routing.md) | unit, security | P5 |
| P5-PRIVACY-001 | Privacy filtering before P4 final masking | astra-ui | [P5 Privacy](../design/p5/p5-privacy-aware-ui.md) | security, integration | P5 |
| P5-SERVICE-001 | Independent capability-protected Spatial UI runtime | astra-spatial-ui-service | [P5 System Design](../design/p5/p5-system-design.md) | contract, integration | P5 |
| P5-SHELL-001 | Shared Phone and Projection spatial workspace model | astra-shell | [P5 System Design](../design/p5/p5-system-design.md) | QML, graphics | P5 |
| P5-TEST-001 | Measured P5 release gate with real Metal evidence | P5 verification | [P5 Test Design](../design/p5/p5-test-design.md) | release gate | P5 |
