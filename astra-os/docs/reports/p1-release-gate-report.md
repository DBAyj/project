# P1 Release Gate Report

## Execution Context

- Executed: 2026-07-14 (Asia/Shanghai)
- Project: `/Users/apple/CodexProjects/astra-os`
- Release-candidate branch: `feature/p1-dual-display-simulator`
- Release-candidate code commit: `70ab9b1`
- Version: `0.1.0-alpha.1`
- Host: macOS 27.0 (build 26A5378j), arm64
- Qt: 6.11.1; CMake: 4.4.0; Ninja: 1.13.2
- Binary: `runtime/tmp/p1-build/apps/astra-shell/astra-shell`, Mach-O arm64

## Release-Gate Evidence

- Formatting: PASS. The tracked-source whitespace checker passed.
- Static checks: PASS. ShellCheck and Python bytecode compilation passed.
- Build and architecture: PASS. Qt Quick, Controls, and Quick 3D linked from arm64 Homebrew frameworks; no Intel Homebrew dependency was found.
- CTest: PASS, 11/11.
- Schema contracts: PASS, 4/4.
- QML tests: PASS, 9/9, with 0 failures and 0 skips.
- Graphics verifier tests: PASS, 4/4.
- Policy and error-registry focused tests: PASS, 1/1 each.
- Runtime configuration test: PASS, 1/1 CTest plus Phone warning QML coverage.
- Audit Schema: PASS. A fresh 920-record JSONL batch validated in full; it contains 101 each of `projection_started`, `projection_paused`, `projection_resumed`, and `projection_stopped`, plus one denial.
- Accessibility: PASS. Project QML warning count is 0 and the 9 QML cases verify accessible control and status metadata.
- Metal: PASS. Both native windows reported `graphics_api=Metal`, `renderer_interface=RHI`, and `rhi_backend=Metal`; QSG independently logged QRhi Metal creation, Apple M1 Pro device selection, and CAMetalLayer creation for both windows.
- Dual-window interaction: PASS. Startup rejects incomplete root-window creation. A CTest uses the real `ShellController` and real Phone/Projection QML components to exercise task execution, drag rotation, wheel zoom, pause/resume, reset, fullscreen `QWindow` visibility, stop, and denial. QML pointer events are covered separately.
- Frozen documentation verifier: PASS.

## Findings

- Failed items: none.
- Gate warnings: none.
- Evidence limitation: no screenshot artifact was generated because the unbundled executable could not be addressed by the available desktop-capture integration. This is not a release-gate warning because the Qt runtime supplied independent first-frame and Metal-layer evidence, and automated component interaction passed.

## Conclusion

`P1_RELEASE_GATE_PASSED`

The release candidate was integrated into `develop` by `b45c547`, and the full gate passed again on that merge baseline. No P2 branch, service, model route, or cloud integration was created.
