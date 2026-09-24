# P1.1 Window Interaction Evidence

- Executed: 2026-07-14 (Asia/Shanghai)
- Command: `make p1-test-window-interaction`
- Result: `WINDOW_INTERACTION_EVIDENCE_PASSED`

The application loads `PhoneWindow` and `ProjectionWindow` as two QML root windows in one Qt process. It rejects startup unless the two expected root-window titles are present. QSG reported a Metal QRhi and CAMetalLayer for each titled window. A dedicated CTest created real Phone and Projection components with a real `ShellController`, exercised execution, drag rotation, wheel zoom, pause/resume, reset view, enter/exit fullscreen with `QWindow` visibility assertions, stop, and `NO_PROJECTION` denial, and asserted public state. The QML suite separately used pointer events to verify control semantics and safe scene visibility.

No screenshot is included: the native unbundled test executable could not be addressed by the available desktop-capture integration. The release gate uses Qt window creation, first-frame RHI output, and QML pointer interaction as the runtime evidence path.
