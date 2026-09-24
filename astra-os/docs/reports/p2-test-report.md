# P2 Test Report

The unified P2 suite executed non-empty public-interface groups:

| Group | Passed | Failed | Skipped |
| --- | ---: | ---: | ---: |
| Python unit | 34 | 0 | 0 |
| P2 contract | 5 | 0 | 0 |
| Python integration | 13 | 0 | 0 |
| Python security | 10 | 0 | 0 |
| CTest C++/Qt | 14 | 0 | 0 |
| QML | 14 | 0 | 0 |
| Performance and stability | 2 | 0 | 0 |
| Live service E2E | 1 | 0 | 0 |

The tests cover all declared public seams: strict protocol/configuration schemas, immutable domain values, normalization/security, rules, deterministic model, fusion, slots, ambiguity, confirmation expiry/replay, cache, metrics, audit JSONL, dual-engine fallback, JSON-RPC Socket, development HTTP/WebSocket, C++ Shell client, P2 controller execution, QML analysis/confirmation, 20-request concurrency, and 10,000-request stability.

P1 regressions remain part of final acceptance through `make p1-test` and `make p1-verify`. No empty or skipped test is counted.

Test result: `PASSED`.
