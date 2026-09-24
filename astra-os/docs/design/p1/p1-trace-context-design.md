# P1 Trace Context

P1 represents a user operation with `OperationContext`: `traceId`, `requestId`, `sessionId`, `actor`, and `privacyLevel`. `IntentSimulator` generates one trace ID and request ID for a submitted task. The controller retains the active session ID after a successful projection start and reuses it for pause, resume, reset, fullscreen, and stop events.

Every audit record includes `schema_version`, `timestamp`, `level`, `service`, `module`, `event`, `trace_id`, `request_id`, `session_id`, `actor`, `privacy_level`, `result`, `error_code`, `message`, and `details`. Session IDs and error codes may be JSON null when no session or error exists; the field remains present. Required event fields are validated by `audit-event-v1.schema.json`.

The controller integration test verifies start-request/start trace continuity, pause/resume/stop session continuity, and mandatory audit fields. The release gate generates a fresh 920-record batch and validates every record against the Schema.
