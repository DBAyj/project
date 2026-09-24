# P5 Graphics Evidence

Baseline: `P4_RELEASE_BASELINE_FINAL`

Result: `P5_GRAPHICS_PASSED`

Both real Qt windows rendered through Metal/RHI, exposed visible spatial workspaces, and produced nonblank captured pixels. The Projection window also retained the P4 fixture first frame.

- Phone first frame: 401 ms
- Projection first frame: 431 ms
- Phone screenshot: {'width': 952, 'height': 1876, 'variance': 1321.4229770581724, 'sampled_colors': 1558}
- Projection screenshot: {'width': 2784, 'height': 1728, 'variance': 2205.844371408922, 'sampled_colors': 531}
- Metal backend: `Metal` through Qt `RHI` for both windows
- P4 projection first frame: visible
- P4 readable public labels: ['P5 PUBLIC fixture task']
- Private labels in P4 output: none
- Restored state privacy: policy reclassified, projection remained safely cleared
- Fullscreen enter/exit cycle: passed
- Project QML warnings: 0

Evidence: [Phone screenshot](evidence/p5-phone.png), [Projection screenshot](evidence/p5-projection.png).
