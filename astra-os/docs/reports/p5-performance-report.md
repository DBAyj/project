# P5 Performance Evidence

Baseline: `P4_RELEASE_BASELINE_FINAL`

Result: `P5_PERFORMANCE_PASSED`

Measured on the local Apple Silicon development host with 100 visible fixture components. The first-frame value in this report measures the framework layout and P4 layer batch; the actual Metal UI first frame is recorded by the graphics gate.

- Layout P95: 0.001500 ms (1000 iterations)
- Input routing P95: 0.000500 ms (10000 iterations)
- Focus switching P95: 0.003875 ms (5000 iterations)
- State update P95: 0.000083 ms
- ProjectionLayer batch P95: 0.415250 ms (1000 iterations)
- Framework first frame: 0.416042 ms
- Mean framework FPS: 2416.937
- Stable framework FPS floor (frame P95): 2391.155
