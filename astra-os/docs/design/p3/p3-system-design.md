# P3 System Design

`astra-spatial-service` is an independent C++20 process. A source adapter supplies immutable frames to preprocessing, detection, scoring, mapping, and scene coordination. The service owns spatial state and emits JSON-RPC results and bounded events; `astra-shell` owns presentation and coordinates target-loss safety with the existing projection policy/state services.

The core library depends on Qt Core/Gui/Multimedia/Network and narrowly selected OpenCV modules. Host-specific camera behavior stays in `CameraFrameSource`; domain, mapping, scene, protocols, and tests remain portable to ARM64 Linux. The default source is deterministic simulation.

Thread ownership is explicit: Qt owns device callbacks and transport event loops; a bounded worker step processes the newest frame and drops stale frames rather than queueing unbounded video. Health proves process liveness, readiness proves valid configuration and at least simulation availability.
