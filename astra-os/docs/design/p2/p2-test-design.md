# P2 Test Design

Public seams are strict schemas, dataclass domain values, normalizer/security preprocessor, rule engine, model adapter, candidate merger, slot extractor, ambiguity/confirmation policy, cache, audit JSONL, pipeline/service methods, JSON-RPC socket, development HTTP, and Shell client/fallback behavior. Tests assert public input/output and state rather than private implementation.

Unit coverage includes the 30 required domain cases. Contract tests provide valid and invalid examples for all five protocol schemas and four configuration schemas. Integration covers service calls, confirmation, safe non-execution, engine fallback, transport, audit trace, and P1 state regression. Security tests cover injection, sensitive patterns, limits, malformed payloads, expiry/replay, and socket permissions. Performance/stability tests produce machine-readable results used by reports.
