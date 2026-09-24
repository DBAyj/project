# ADR-0001: Use Qt for Cross-Platform UI

- Status: Accepted
- Date: 2026-07-14

## Context

AstraOS needs phone, projection, and spatial UI that can develop on macOS and deploy on ARM64 Linux without a host-framework dependency.

## Decision

Use Qt 6 with QML for presentation and C++ for Qt integration and rendering-adjacent work.

## Reasons and Alternatives

Qt provides a shared graphics and QML stack across the selected host and target. AppKit/SwiftUI are rejected because they bind the product core to macOS. A web-only shell is rejected because it weakens the real-time spatial integration requirement.

## Impact, Risks, and Rollback

Qt version and target-driver variation require compatibility testing. The UI contract remains service- and schema-driven, allowing a future frontend replacement while preserving service APIs.
