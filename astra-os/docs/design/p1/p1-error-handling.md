# P1 Error Handling

P1 registers and returns 1002, 1003, 2001, 4001-4005, 4301, 4302, 5001, and 9001 through `ErrorCodeRegistry`. Every descriptor has a stable symbol, Chinese and English message, owning module, severity, retryability, and audit requirement. Unknown values map to 9001. The registry exports JSON for documentation and conflict tests; user-facing services query it instead of duplicating error text.

Failures surface a clear message, write redacted audit/log context, preserve the phone window, and clear projection output first. Invalid configuration selects built-in safe defaults, publishes `INVALID_FALLBACK_ACTIVE` or `LOAD_FAILED_FALLBACK_ACTIVE`, and displays a warning in the Phone Display status panel. Audit-write failure remains visible in system state without crashing the application. A graphics or window failure does not expose retained projection content; the release gate requires actual RHI initialization evidence before declaring Metal support.
