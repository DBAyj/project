# AstraOS Risk Register

| ID | Risk | Probability | Impact | Level | Trigger | Mitigation | Contingency | Owner | Validation phase |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| R-001 | Projection brightness is insufficient | Medium | High | High | Scene unreadable in target room | Measure luminance and use target qualification | Restrict scenarios and revise optics | astra-projection | P8 |
| R-002 | Projection power draw is excessive | Medium | High | High | Battery budget exceeded | Profile power and apply output policy | Disable projector under threshold | astra-projection | P8 |
| R-003 | Thermal load throttles the terminal | Medium | High | High | Sustained thermal warning | Instrument thermal policy | Degrade AI/rendering and close projection | astra-hal | P8 |
| R-004 | Gesture recognition is inaccurate | High | Medium | High | False action rate exceeds acceptance | Confidence threshold and confirmation | Fall back to touch/voice | astra-input | P3 |
| R-005 | Single-viewer 3D limits experience | High | Medium | High | More than one active viewer | State single-viewer boundary in UI | Use 2D shared mode | astra-spatial | P3 |
| R-006 | AI issues an unintended action | Medium | Critical | Critical | Low-confidence or unexpected tool call | Clarification, capability checks, audit | Cancel task and revoke capability | astra-task-engine | P2 |
| R-007 | Private information is projected | Low | Critical | High | Privacy-policy violation | Mandatory policy gate and labels | Close output and write incident audit | astra-policy | P4 |
| R-008 | Application ecosystem is insufficient | High | Medium | High | MVP lacks useful integrations | Narrow initial Skills and SDK | Prioritize first-party workflows | astra-sdk | P6 |
| R-009 | macOS/Linux behavior differs | Medium | High | High | Target build or behavior divergence | Portable HAL and CI target testing | Block host-only merge | astra-hal | P7 |
| R-010 | Qt changes affect compatibility | Medium | Medium | Medium | Upgrade breaks rendering/API | Pin support range and contract tests | Hold version and patch adapter | astra-shell | P1 |
| R-011 | Linux drivers are incomplete | Medium | High | High | Device unavailable on board | Early driver inventory and HAL fakes | Change hardware selection | astra-hal | P7 |
| R-012 | ARM64 performance is inadequate | Medium | High | High | Telemetry misses target budget | Profile representative workloads | Degrade quality or select hardware | astra-ai-runtime | P7 |
| R-013 | Model artifacts exceed storage or memory | Medium | High | High | Model cannot load | Descriptor sizing and model routing | Cloud route for approved data | astra-model-router | P2 |
| R-014 | Network dependency harms availability | Medium | Medium | Medium | Cloud route unavailable | Local model fallback and explicit status | Queue noncritical task | astra-model-router | P2 |
| R-015 | System update fails | Low | Critical | High | Verification/boot failure | Signed A/B staging and health check | Automatic rollback and recovery mode | astra-update | P5 |
| R-016 | Supply-chain dependency fails | Medium | High | High | Component or provider unavailable | Dual-source evaluation and manifests | Re-scope prototype hardware | astra-device | P8 |
| R-017 | Prompt-injection phrase bypasses basic detection | Medium | High | High | Adversarial text reaches a risky action | Layered policy, deny-by-default execution, confirmation, redacted audit | Reject action and update versioned signals | astra-security | P2 |
| R-018 | Intent ambiguity causes an unintended projection action | Medium | Critical | Critical | Candidate delta or slots are inconclusive | Configured thresholds, clarification, confirmation, P1 policy gate | Keep projection unchanged and record denial | astra-intent | P2 |
| R-019 | Intent Service process or IPC becomes unavailable | Medium | Medium | Medium | Health/readiness or Socket request fails | Independent process, health checks, rule/model fallback | Shell permits only stop, hide, and status safe rules | astra-intent | P2 |
| R-020 | Spatial UI input penetrates a safety overlay | Low | Critical | High | Background component receives blocked input | Central hit ordering and system-layer preemption | Cancel interaction and retain system focus | astra-ui | P5 |
| R-021 | Destroyed or hidden component retains focus | Low | High | High | Focus owner is unavailable after lifecycle change | Unregister/release focus on hide and destroy | Reset focus scope to safe default | astra-ui | P5 |
| R-022 | P5 privacy filtering diverges from P4 final mask | Low | Critical | Critical | Private component produces a projection layer | Filter before mapping and retain P4 final pass | Target-loss safe clear and P1 fallback | astra-ui | P5 |
| R-023 | Simulated P3 data is mistaken for physical spatial evidence | Medium | High | High | Reports omit environment qualification | Mandatory evidence classification | Block physical-space claims without hardware evidence | astra-spatial-ui | P5 |
