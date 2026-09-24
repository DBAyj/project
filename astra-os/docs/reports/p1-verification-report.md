# P1 Verification Report

- `make p1-verify`: PASS.
- `make p1-run`: the project PID was created, remained alive, and was stopped by `make p1-stop`; only the project PID was targeted.
- Binary: `runtime/tmp/p1-build/apps/astra-shell/astra-shell` is Mach-O `arm64`.
- Qt graphics evidence: QSG created QRhi Metal backends for both titled windows, reported `Metal device: Apple M1 Pro`, created CAMetalLayer surfaces, and the application logged `graphics_api=Metal`, `renderer_interface=RHI`, and `rhi_backend=Metal`.
- Window evidence: Phone and Projection windows each produced a native title and a rendered first frame. A dedicated CTest used the real controller and real QML components for execution, drag rotation, wheel zoom, pause/resume, reset, fullscreen `QWindow` visibility, stop, and denial; QML pointer tests independently verified control-event semantics.
- QML runtime log: project QML warning count is 0.
- Audit evidence: a fresh 920-record batch is valid JSONL, conforms to the audit Schema, includes required projection events, and has no password, token, secret, API-key, or private-key marker.
- Screenshot evidence: no image was produced because the unbundled executable could not be addressed by the available desktop-capture integration. This is not used as a substitute for runtime evidence.

This is a P1.1 release-gate verification record. It does not enter Phase P2.
