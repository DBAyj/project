# Phase P3 Spatial Requirements

## Scope

P3 adds a portable spatial service that consumes camera, image, video, or deterministic simulation frames and produces typed planar candidates, calibration, coordinate mappings, scene state, observer state, and safe projection-target state. It does not implement SLAM, depth sensing, gesture/face/eye tracking, real projector control, persistent maps, or P4 render warping.

## Requirements

| ID | Requirement | Acceptance seam | Stage |
| --- | --- | --- | --- |
| P3-CAPTURE-001 | Enumerate and start/stop supported frame sources. | `FrameSource` and service source API | P3 |
| P3-CAPTURE-002 | Fall back to simulation when camera authorization or hardware is unavailable. | service integration | P3 |
| P3-VISION-001 | Deterministically preprocess 1280x720 frames and detect planar quadrilateral candidates. | OpenCV pipeline | P3 |
| P3-SURFACE-001 | Score, order, and select only policy-eligible projection surfaces. | candidate selector | P3 |
| P3-CALIBRATION-001 | Validate ordered four-point calibration and compute invertible homographies. | calibration service | P3 |
| P3-COORDINATE-001 | Every point, matrix, anchor, and target identifies its coordinate system. | domain types and schemas | P3 |
| P3-SCENE-001 | Maintain scene, object, anchor, target, and observer lifecycle state. | scene graph | P3 |
| P3-IPC-001 | Expose health, state, source, detection, selection, calibration, observer, and reset operations over local JSON-RPC. | transport contract | P3 |
| P3-UI-001 | Show spatial status and controls on Phone Display and target/debug state on Projection Display. | QML and controller integration | P3 |
| P3-SAFETY-001 | Clear sensitive projection content and pause safely when the selected target is lost or invalid. | Shell safety integration | P3 |
| P3-PRIVACY-001 | Do not persist raw frames unless explicit debug-frame configuration enables project-local storage. | configuration and audit | P3 |
| P3-AUDIT-001 | Emit schema-valid, traceable spatial lifecycle and safety events without raw image content. | JSONL audit contract | P3 |
| P3-PERF-001 | Meet the measured preprocessing, detection, pipeline, FPS, and event-latency budgets. | performance suite | P3 |
| P3-STABILITY-001 | Complete required frame, selection, calibration, source, and lifecycle loops without illegal state or resource residue. | stability suite | P3 |

## Acceptance Policy

Simulation and image fixtures are mandatory in every environment. Camera tests report `PASSED` only with real enumeration, authorization, frame receipt, stop, and release evidence; otherwise they report `SKIPPED_ENVIRONMENT_LIMITATION`. P1 and P2 remain regression gates. All errors use the central registry and all public JSON values use strict schemas.
