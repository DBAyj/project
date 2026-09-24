# ADR-0008: Use OpenCV For P3 Surface Detection

- Status: Accepted
- Date: 2026-07-15

## Context
P3 needs portable, deterministic image preprocessing, contours, quadrilateral geometry, and homography without a large vision model.

## Decision
Use narrowly linked OpenCV core, image processing, image codec, video I/O, and calibration modules behind P3 C++ interfaces.

## Reasons And Alternatives
OpenCV is proven on macOS and ARM64 Linux. Hand-written image kernels and platform-only vision frameworks were rejected.

## Impact, Risks, And Rollback
The build gains a versioned native dependency and must isolate OpenCV types from public protocols. Rollback disables P3 and preserves P2.
