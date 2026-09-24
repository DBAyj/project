# P4 Safety And Test Design

## Safety Rules

Only policy-provided privacy values may enter render requests. The renderer does
not infer privacy. `NO_PROJECTION` and `PRIVATE_SCREEN_ONLY` select full clear.
Target expiry, invalid Homography, graph/pass error, output disconnect, or
service exception perform clear before any further present. Clear overwrites the
offscreen target and output, then records an audit event with a registered code.

## Test Layers

- Unit: session transitions, layer order, graph ordering, geometry, mesh,
  crop/overscan, color, privacy, and safe clear through public C++ interfaces.
- Contract: all public JSON schemas, registry entries, methods, and audit data.
- Visual: fixture-generated images compare known points/checksums for geometry,
  color, masking, and composition; a service visual baseline decodes the
  `projection.output.frame` PNG and checks its rendered pixels and raw digest.
- Graphics: RHI backend plus scene/output initialization, visible-window, and
  first-frame evidence; backend environment variables alone are insufficient.
- Integration: JSON-RPC capability checks, P1 Shell manual controls, fallback,
  target-loss simulation, and service run/stop.
- Performance/stability: report P95, FPS, first frame, clear latency, and
  repeat lifecycle/render cycles.

## Release Conditions

The release gate fails for a privacy-mask bypass, failed safe clear, retained
old frame, unverified Metal evidence, graph output after failure, invalid
Homography output, P1 regression, or a missing fixture-only marker.
