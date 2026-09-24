# ADR-0011: Use Session-Only Spatial Anchors In P3

- Status: Accepted
- Date: 2026-07-15

## Context
Persistent anchors require SLAM, map identity, migration, and privacy controls outside P3.

## Decision
Create UUID anchors only for the active spatial-service session and mark `persistent=false` in domain values and schemas.

## Reasons And Alternatives
Session anchors validate scene ownership without inventing unsupported localization. Persistent files and cloud anchors were rejected.

## Impact, Risks, And Rollback
Restart requires redetection or calibration. Rollback clears only session state and preserves calibration profiles.
