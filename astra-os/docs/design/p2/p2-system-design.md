# P2 System Design

`astra-shell` sends an `intent-request-v2` through `IntentServiceClient` to `astra-intent-service`. Production-local transport is JSON-RPC 2.0 over a Unix Domain Socket; explicit development mode also exposes HTTP and lightweight event streaming. The service owns input normalization, security preprocessing, rules, deterministic local classification, candidate fusion, slot extraction, confidence, ambiguity, confirmation, cache, metrics, and audit emission.

The service is a separate Python 3.12 process and uses only portable standard-library networking plus PyYAML and jsonschema. It does not import Qt or platform frameworks. Shell receives a typed result and coordinates existing P1 policy/session services. Projection QML never calls the intent service. If the service is unavailable, Shell permits only stop projection, hide projection content, and show system status through its P1 safety rules.

Health is independent from readiness: health proves the process loop is alive; readiness requires valid configuration, rules, cache, and at least one classification engine. Every request receives one trace ID and one request ID across transport, pipeline, audit, and Shell execution.
