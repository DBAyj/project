# P4 Projection Rendering Engine Plan

## Test Seams

The requirements pre-agree these public seams: `ProjectionSession`,
`ProjectionOutput`, `ProjectionLayer`, `RenderGraph`, geometry validation and
warp, final privacy/safe-clear composition, local projection-service JSON-RPC,
and the Shell manual-control client. Tests use deterministic fixtures and never
reach P3 private classes.

## Work Items

- [ ] 1. Preflight audit and P4 boundary
  - Paths: `docs/reports/p4-preflight-audit.md`, `docs/requirements/phase-p4-requirements.md`.
  - Test first: `tests/contract/test_p4_schemas.py` checks the fixture-only marker.
  - Implement: document P3 limits, P1 fallback, scope, and release-blocking conditions.
  - Verify: `make docs-verify`, `make p4-test-contract`.
  - Commit: `chore: prepare P4 projection rendering engine development`.
  - Rollback: revert the preparation commit; P1-P3 sources remain untouched.

- [ ] 2. Protocols and schemas
  - Paths: `protocols/projection/p4-*.schema.json`, `protocols/error-codes.yaml`, `protocols/method-registry.yaml`, `tests/contract/test_p4_schemas.py`.
  - Test first: strict-schema and error-registry contracts reject missing and extra fields.
  - Implement: versioned request, output, session, layer, graph, warp, mask, and audit schemas.
  - Verify: `make p4-test-contract`.
  - Commit: `feat: add P4 projection rendering protocols`.
  - Rollback: revert the protocol commit as a unit.

- [ ] 3. ProjectionOutput abstraction
  - Paths: `libraries/astra-render/include/astra/render/ProjectionOutput.h`, `libraries/astra-render/tests/projection_output_test.cpp`.
  - Test first: unavailable output rejects initialize/present and enters a safe state.
  - Implement: portable initialize, present, clear, shutdown, state, resolution, and refresh-rate interface.
  - Verify: `make p4-test-unit`.
  - Commit: `feat: add projection output abstraction`.
  - Rollback: remove render-library target and its protocol use.

- [ ] 4. ProjectionSession state machine
  - Paths: `runtime/projection/ProjectionSession.{h,cpp}`, `runtime/projection/tests/projection_session_test.cpp`.
  - Test first: legal transitions pass; illegal transitions audit `INVALID_PROJECTION_TRANSITION`.
  - Implement: `IDLE` through `ERROR` lifecycle with fail-closed stop/clear paths.
  - Verify: `make p4-test-unit`.
  - Commit: `feat: add projection runtime session management`.
  - Rollback: retain P1 session service as the active Shell fallback.

- [ ] 5. Layer system
  - Paths: `libraries/astra-render/include/astra/render/ProjectionLayer.h`, `libraries/astra-render/tests/projection_layer_test.cpp`.
  - Test first: normal content cannot sort above `PRIVACY_MASK`.
  - Implement: fixed layer types and stable composition ordering.
  - Verify: `make p4-test-unit`.
  - Commit: `feat: add projection layer composition model`.
  - Rollback: disable P4 composition and clear output.

- [ ] 6. Render graph
  - Paths: `libraries/astra-render/include/astra/render/RenderGraph.h`, `libraries/astra-render/src/RenderGraph.cpp`, `libraries/astra-render/tests/render_graph_test.cpp`.
  - Test first: topological order succeeds and cycles/missing dependencies fail closed.
  - Implement: fixed P4 passes and pass-failure propagation to safe clear.
  - Verify: `make p4-test-unit`.
  - Commit: `feat: add projection render graph`.
  - Rollback: skip graph creation and invoke final clear.

- [ ] 7. Offscreen rendering
  - Paths: `libraries/astra-render/src/OffscreenRenderTarget.cpp`, `libraries/astra-render/tests/offscreen_render_test.cpp`.
  - Test first: source pixels render to a sized target without source mutation.
  - Implement: Qt RHI-backed portable offscreen-target abstraction.
  - Verify: `make p4-test-graphics`.
  - Commit: `feat: add offscreen projection rendering`.
  - Rollback: force `NO_PROJECTION` and clear the P1 projection window.

- [ ] 8. Homography warp
  - Paths: `libraries/astra-render/include/astra/render/Homography.h`, `libraries/astra-render/src/Homography.cpp`, `libraries/astra-render/tests/homography_test.cpp`.
  - Test first: front/keystone fixtures pass; invalid, self-intersecting, and non-invertible inputs fail.
  - Implement: normalized DLT validation and deterministic pixel mapping for valid convex quadrilaterals.
  - Verify: `make p4-test-unit`, `make p4-test-visual`.
  - Commit: `feat: add homography geometry correction`.
  - Rollback: reject warps and perform safe clear.

- [ ] 9. Mesh warp
  - Paths: `libraries/astra-render/include/astra/render/MeshWarp.h`, `libraries/astra-render/src/MeshWarp.cpp`, `libraries/astra-render/tests/mesh_warp_test.cpp`.
  - Test first: a regular grid passes and a folded cell fails validation.
  - Implement: configurable bounded mesh geometry after Homography.
  - Verify: `make p4-test-unit`, `make p4-test-visual`.
  - Commit: `feat: add configurable projection mesh warp`.
  - Rollback: omit mesh pass while retaining Homography/safe clear.

- [ ] 10. Crop and overscan
  - Paths: `libraries/astra-render/src/CropOverscan.cpp`, `libraries/astra-render/tests/crop_overscan_test.cpp`.
  - Test first: crop bounds and overscan clamp independent known grids.
  - Implement: normalized crop rectangle and bounded overscan pass.
  - Verify: `make p4-test-visual`.
  - Commit: `feat: add projection crop and overscan processing`.
  - Rollback: use identity crop and zero overscan.

- [ ] 11. Color and gamma compensation
  - Paths: `libraries/astra-render/src/ColorCompensation.cpp`, `libraries/astra-render/tests/color_compensation_test.cpp`.
  - Test first: gamma and gray-wall literals produce expected reference pixels.
  - Implement: bounded gamma and RGB gain compensation in linear color space.
  - Verify: `make p4-test-visual`.
  - Commit: `feat: add projection color compensation`.
  - Rollback: use identity color compensation.

- [ ] 12. Privacy mask
  - Paths: `libraries/astra-render/include/astra/render/PrivacyMask.h`, `libraries/astra-render/src/PrivacyMask.cpp`, `libraries/astra-render/tests/privacy_mask_test.cpp`.
  - Test first: full, rectangle, polygon, and solid masks overwrite prior content.
  - Implement: final-pass masks controlled only by policy-supplied privacy level.
  - Verify: `make p4-test-security`.
  - Commit: `security: add final-pass projection privacy masking`.
  - Rollback: force full clear for every request.

- [ ] 13. Safe clear
  - Paths: `runtime/projection/SafeClearController.{h,cpp}`, `runtime/projection/tests/safe_clear_test.cpp`.
  - Test first: `NO_PROJECTION`, target loss, output loss, and pass failures erase the previous frame.
  - Implement: atomic clear-and-stop handoff with audit events and no retained output frame.
  - Verify: `make p4-test-security`, `make p4-test-integration`.
  - Commit: `security: enforce P4 safe clear on output faults`.
  - Rollback: delegate to P1 stop projection and hide content.

- [ ] 14. Animation and frame scheduling
  - Paths: `runtime/projection/FrameScheduler.{h,cpp}`, `runtime/projection/tests/frame_scheduler_test.cpp`.
  - Test first: overload reduces scheduling rate without bypassing privacy/safe-clear work.
  - Implement: deadline scheduler, pause/resume, and frame-drop audit.
  - Verify: `make p4-test-performance`.
  - Commit: `feat: add projection animation and frame scheduling`.
  - Rollback: render only explicit static frames.

- [ ] 15. Independent projection service
  - Paths: `services/astra-projection-service/`, `services/astra-projection-service/tests/`, `runtime/projection/sockets/`.
  - Test first: capability denial and output failure return schema-valid errors and clear output.
  - Implement: local Unix-socket JSON-RPC service owning runtime session and render graph.
  - Verify: `make p4-test-integration`, `make p4-run`.
  - Commit: `feat: add standalone AstraOS projection service`.
  - Rollback: stop project-owned P4 process and restore P1 Shell controls.

- [ ] 16. Shell integration
  - Paths: `apps/astra-shell/src/clients/ProjectionServiceClient.*`, `apps/astra-shell/src/controllers/ShellController.*`, `apps/astra-shell/tests/integration/p4_projection_integration_test.cpp`.
  - Test first: manual start/pause/resume/stop and fullscreen work through the public client.
  - Implement: P4 client with P1 fallback retained for denial and service failure.
  - Verify: `make p4-test-integration`, `make p1-test`.
  - Commit: `feat: connect Astra Shell to projection runtime`.
  - Rollback: select existing `ProjectionSessionService`.

- [ ] 17. P2/P3 adapters
  - Paths: `runtime/projection/P2IntentAdapter.*`, `runtime/projection/P3SpatialAdapter.*`, `runtime/projection/tests/adapter_test.cpp`.
  - Test first: fixture target adapts; absent P2/P3 never creates a real-integration claim.
  - Implement: schema adapters/test doubles with mandatory P3 limitation marker.
  - Verify: `make p4-test-contract`, `make p4-test-integration`.
  - Commit: `feat: add P2 P3 projection adapters`.
  - Rollback: remove adapter registration while manual control remains active.

- [ ] 18. Graphics tests
  - Paths: `tests/p4-e2e/test_p4_graphics_backend.py`, `scripts/verify_p4_graphics.py`.
  - Test first: Metal cannot pass without RHI, scene initialization, visible output, and first-frame evidence.
  - Implement: macOS Metal evidence verifier while keeping renderer source portable.
  - Verify: `make p4-test-graphics`.
  - Commit: `test: add P4 graphics and visual baseline verification`.
  - Rollback: fail the P4 gate; retain P1 graphics gate.

- [ ] 19. Visual baselines
  - Paths: `assets/projection-fixtures/`, `tools/projection-fixture-generator/`, `tests/p4-e2e/test_p4_visual.py`.
  - Test first: generated geometry, color, layer, and privacy images compare to known pixels/checksums.
  - Implement: project-generated assets and deterministic visual comparator.
  - Verify: `make p4-generate-fixtures`, `make p4-test-visual`.
  - Commit: `test: add P4 graphics and visual baseline verification`.
  - Rollback: `make p4-clean` removes only generated build/cache outputs.

- [ ] 20. Security tests
  - Paths: `services/astra-projection-service/tests/security/`, `tests/p4-e2e/test_p4_security.py`.
  - Test first: privacy bypass, invalid capability/Homography, and output faults cannot expose a prior frame.
  - Implement: denial-path and audit assertions through public protocols.
  - Verify: `make p4-test-security`.
  - Commit: `security: verify P4 safe clear and privacy isolation`.
  - Rollback: P4 is disabled and P1 denies external projection.

- [ ] 21. Performance and stability
  - Paths: `services/astra-projection-service/tests/performance/`, `scripts/verify_p4_baseline.py`, `docs/reports/p4-verification-report.md`.
  - Test first: deterministic schedule calculates P95, first-frame, clear latency, and long-cycle limits.
  - Implement: measured performance/stability harness and report generator.
  - Verify: `make p4-test-performance`, `make p4-test-stability`, `make p4-verify`.
  - Commit: `test: add P4 performance and stability verification`.
  - Rollback: retain reports and use P1 offline fallback.

- [ ] 22. Documentation, release gate, and rollback
  - Paths: `docs/design/p4/`, `scripts/verify_p4.sh`, `scripts/release_gate_p4.sh`, `README.md`, `Makefile`.
  - Test first: release gate fails on a mandatory marker, old-frame residue, or missing run/stop evidence.
  - Implement: P4 commands, reports, rollback guide, and final P1/P2/P3 regression evidence.
  - Verify: `make p4-release-gate`.
  - Commit: `docs: complete P4 verification and release gate evidence`.
  - Rollback: `make p4-stop`, `make p4-clean`, and retained P1 policy/session fallback.
