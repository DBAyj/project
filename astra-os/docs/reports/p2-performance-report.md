# P2 Performance Report

Measured on the macOS Apple Silicon host with Python 3.12.13:

| Measure | Result | Target |
| --- | ---: | ---: |
| Service startup | 0.050 s | <= 3 s |
| Rule engine P95 | 0.022 ms | <= 20 ms |
| Full pipeline P95 | 7.178 ms | <= 100 ms |
| Full pipeline average | 6.696 ms | Recorded |
| Full pipeline P99 | 7.443 ms | Recorded |
| Cached pipeline P95 | 0.931 ms | Recorded |
| Health P95 | 0.000 ms | <= 20 ms |
| 20-worker throughput | 158.068 requests/s | Zero errors |
| 20-worker errors | 0 | 0 |
| Performance-run RSS growth | 1.656 MB | No sustained growth |

The stability path completed 10,000 requests in 6.949 seconds, approximately 1,439 requests/s. It produced 40,020 strict audit records with zero JSON/Schema errors, zero request failures, 0.0 MB RSS growth, and a clean Socket after stop.

Machine-readable evidence: `runtime/tmp/p2-performance.json` and `runtime/tmp/p2-stability.json`.

Performance result: `PASSED`.
