# ADR-0019: Map Spatial UI Components To P4 Projection Layers

- Status: Accepted for P5 fixture-mode development
- Date: 2026-07-15

P5 translates privacy-approved component snapshots to public P4
`ProjectionLayer` values. P5 does not access textures, warp, crop, or color
passes. This preserves P4 ownership and its final safe-clear behavior.
