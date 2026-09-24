# 05 IPC Protocol

## Transport Selection

- Local service request/response: Unix Domain Socket using JSON-RPC 2.0.
- Real-time subscription events: WebSocket, normally a local endpoint authenticated by capability token.
- High-frequency graphics data: shared memory contract, deferred beyond this documentation phase. P4 fixture preview uses the bounded local `projection.output.frame` PNG relay defined by ADR-0015; it is not a high-frequency transport.
- Hardware: `astra-hal` typed abstract interface.
- Development diagnostics: HTTP only in explicit development mode and never the public default.

Kafka, RabbitMQ, Kubernetes Service abstractions, service registries, and other broker infrastructure are outside the first generation.

## JSON-RPC Example

```json
{"jsonrpc":"2.0","id":"request-uuid","method":"projection.start","params":{"scene_id":"demo-device","target":"desk","privacy_level":"PUBLIC"},"security_context":{"capability_token":"redacted-token"}}
```

```json
{"jsonrpc":"2.0","id":"request-uuid","result":{"session_id":"projection-session-uuid","status":"ACTIVE"}}
```

```json
{"jsonrpc":"2.0","id":"request-uuid","error":{"code":4301,"message":"Projection denied by privacy policy","data":{"privacy_level":"NO_PROJECTION"}}}
```

The request boundary assigns a `trace_id` when absent and propagates it on every downstream call. Unix sockets bind an operating-system peer identity at connection time, and every request also carries `security_context.capability_token`; `astra-security` validates the token against the peer, method, resource, scope, expiry, and use limit. Schemas: [request](../../schemas/protocols/json-rpc-request.schema.json), [response](../../schemas/protocols/json-rpc-response.schema.json), [event](../../schemas/protocols/websocket-event.schema.json), and the [method registry](../../protocols/method-registry.yaml). The registry binds every frozen public method to strict parameter and result schemas. WebSocket subscriptions begin with capability-checked `event.subscribe` and additionally require the selected event's registry capability. The full compatibility and timeout contract is [Protocol Specification](../protocols/AstraOS-Protocol-Specification-v1.0.md).
