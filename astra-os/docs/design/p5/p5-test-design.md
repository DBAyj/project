# P5 Test Design

Public C++ APIs are the unit seam; strict JSON Schema and JSON-RPC are the
contract seam; the real Shell client/service pair is the integration seam; QML
object behavior and screenshots are the UI seam. Accessibility, Metal, privacy,
performance, stability, process cleanup, audit parsing, and P1/P4 regression
are release blockers. P2/P3 remain explicit unavailable adapters.

Configuration tests load all five real documents, reject missing and invalid
fixtures, and verify that fallback disables external projection. Dual-service
graphics/security tests assert that a readable public label reaches P4 while a
private label does not. Service tests require the full lifecycle audit sequence
and critical focus preemption/restoration payloads.
