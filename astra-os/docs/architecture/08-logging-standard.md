# 08 Logging Standard

Every service writes independently managed structured JSON records with `TRACE`, `DEBUG`, `INFO`, `WARN`, `ERROR`, or `FATAL` level.

```json
{"timestamp":"2026-07-14T08:30:00.000Z","level":"INFO","service":"astra-projection-service","module":"projection.session","event":"projection_started","trace_id":"trace-uuid","request_id":"request-uuid","session_id":"session-uuid","user_id":"local-user","device_id":"device-uuid","result":"success","message":"Projection session started","details":{}}
```

Required fields are `timestamp`, `level`, `service`, `module`, `event`, `trace_id`, `result`, `message`, and `details`. `request_id`, `session_id`, `user_id`, and `device_id` are included when applicable. Passwords, tokens, raw sensitive content, and unstructured context-free failure messages are prohibited. Logs are correlation-aware but audit records remain the security source of truth.
