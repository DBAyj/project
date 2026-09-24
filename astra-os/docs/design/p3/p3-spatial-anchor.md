# P3 Spatial Anchor Design

P3 anchors are session-only transforms with UUID, anchor type, coordinate system, 4x4 transform, quality, persistence=false, and timestamps. They bind scene objects to a selected planar target but are never restored across process sessions.

Removing a target suspends dependent objects and removes or invalidates its anchors. Cross-session, cross-device, SLAM, and world-map persistence are outside P3.
