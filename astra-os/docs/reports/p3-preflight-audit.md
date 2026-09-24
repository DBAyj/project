# P3 Preflight Audit

## Baseline

- Executed: `2026-07-15 07:05:33 CST`
- Project: `/Users/apple/CodexProjects/astra-os`
- Branch: `feature/p3-spatial-scene-engine`
- Source branch: `feature/p2-ai-intent-center`
- Source commit: `5e472ccec6180299f052df6a055adc90308654c4`
- Worktree before P3 changes: clean
- Branch decision: `develop` remains at the P1 baseline, so P3 was created from the locally accepted P2 feature baseline as explicitly permitted by the phase instruction.

## Regression Gate

- P1: passed via `make p1-verify`; CTest 14/14, Schema 4/4, graphics 4/4, QML 14/14, arm64 binary confirmed.
- P2: passed via `make p2-verify`; unit 34/34, contract 5/5, integration 13/13, security 10/10, CTest 14/14, QML 14/14.
- P2 release evidence: `P2_VERIFICATION_PASSED_WITH_SCREENSHOT_WARNING`; no functional failure carried into P3.

## Host And Toolchain

- Host: macOS 27.0 build 26A5378j, Apple Silicon arm64.
- CPU architecture: arm64.
- Apple Clang: 17.0.0.
- CMake: 4.4.0.
- Ninja: 1.13.2.
- Python: 3.12.13.
- Qt, Qt Multimedia, Qt Quick 3D: 6.11.1.
- OpenCV: 5.0.0, available through `lib/cmake/opencv5/OpenCVConfig.cmake` and `lib/pkgconfig/opencv5.pc` under the Homebrew prefix.
- OpenCV install note: Homebrew reported a pre-existing NumPy command-link conflict, but completed and linked the OpenCV formula; C++ OpenCV discovery is independent of those Python command links.
- Disk: 460 GiB volume, 132 GiB available at preflight.

## Camera And Display

- Enumerated camera devices: 1 (`FaceTime高清相机`, unique ID `47B4B64B-7067-4B9C-AD2B-AE273A71F4B5`).
- Camera permission: not requested during read-only preflight; runtime camera tests must report the actual authorization result and fall back to simulation on denial.
- Display: built-in 3024x1964 Retina display, scale-aware Qt path already verified in P1/P2.
- GPU: Apple M1 Pro, 16 cores, Metal 4.

## Existing Spatial Baseline

- Frozen owner: `astra-spatial` for coordinate mapping, anchors, observer pose, and scene objects.
- Existing interfaces: no P3 implementation or spatial protocol directory existed at preflight.
- Existing IPC: JSON-RPC 2.0 over Unix Domain Socket; development HTTP and WebSocket events are permitted only with explicit capability protection.
- Existing error registry: `protocols/error-codes.yaml` and C++ `ErrorCodeRegistry` own stable codes; P3 must add the 3001-3504 ranges there.
- Existing safety path: P1 projection privacy policy/state machine and P2 stop/hide/status offline fallback must remain authoritative.

## Preflight Result

`P3_PREFLIGHT_PASSED_WITH_CAMERA_PERMISSION_PENDING`

The environment is ready for simulation/image-first P3 development. Real-camera acceptance remains conditional on the runtime authorization result and cannot replace deterministic CI fixtures.
