# AstraOS Protocol Specification v1.0

## Envelope and Socket Naming

Local request/response uses JSON-RPC 2.0 UTF-8 messages over Unix Domain Sockets. Socket names use `/run/astra-os/<service>.sock` on target platforms and a platform-adapter equivalent on development hosts. Methods use lowercase dotted names such as `projection.start`; `id` is a caller-generated UUID and `trace_id` propagates across every downstream call. The Unix-socket acceptor records OS peer identity, then `astra-security` binds it to the required `security_context.capability_token`; a missing, mismatched, expired, exhausted, or unauthorized token returns a registered 5000-range error.

## Request, Response, and Events

Requests require `jsonrpc`, `id`, `method`, object `params`, and `security_context.capability_token`; the service boundary assigns `trace_id` when a client lacks one. Success returns `result` only; failure returns `error.code`, `error.message`, and redacted `error.data` only. The exact envelope schemas are [request](../../schemas/protocols/json-rpc-request.schema.json), [response](../../schemas/protocols/json-rpc-response.schema.json), and [event](../../schemas/protocols/websocket-event.schema.json). The [method registry](../../protocols/method-registry.yaml) binds P1 control methods plus P4 `projection.render`, `projection.output.clear`, fixture-only `projection.output.frame`, and `projection.session.command` to strict parameter/result schemas; shutdown is supervisor-only. `projection.output.frame` requires `projection.output.read`, returns only the latest PUBLIC fixture PNG, and fails after safe clear. The [event registry](../../protocols/event-registry.yaml) binds every declared WebSocket event to a strict payload schema and subscription capability. WebSocket events carry `event`, `timestamp`, `trace_id`, and `payload`; clients create subscriptions only through capability-checked `event.subscribe` and must satisfy both `event.subscribe` and the selected event's subscription capability.

## Timeout, Retry, Idempotency, and Errors

Every method declares a bounded timeout in its future interface schema. Transport failure is distinct from application error. The caller may retry only idempotent queries or methods with a stable idempotency key. Start/stop/update methods persist a request outcome keyed by request ID before retrying. Errors use [the central registry](../../protocols/error-codes.yaml); no service invents an unregistered code.

## Version Negotiation and Compatibility

Each service announces protocol major/minor version in `system.health`. Major mismatch rejects the request with a documented compatibility error; a same-major higher minor must ignore unknown optional fields only where the specific schema permits them. Configuration, SDK, model, and data versions negotiate independently. Shared-memory graphics and external network protocols remain future versioned extensions rather than implicit behavior.
