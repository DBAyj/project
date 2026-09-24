# P1 UI Design

Phone Display is 430x860 (minimum 380x720) and has status, task input, privacy selector, result, recent tasks, and system state. Projection Display is 1280x720 (minimum 800x450), starts windowed, and has session header, privacy badge, safe empty state, Quick 3D scene, and controls.

All colors, metrics, and motion durations come from `AstraTheme.qml`. Phone data is private. Projection content appears only for active PUBLIC, ROOM_ONLY, or simulated-authorized sessions. Interactive controls expose `Accessible.name`, `Accessible.description`, and an appropriate role through shared controls; error, privacy, and status surfaces expose accessible static text. QML tests verify pointer execution, pause/resume, reset, fullscreen, stop, and configuration-warning visibility. Chinese text uses Qt's portable system-font selection.
