# 09 Error Code Standard

Error codes are four-digit, stable, machine-readable identifiers registered only in [error-codes.yaml](../../protocols/error-codes.yaml).

| Range | Domain |
| --- | --- |
| 1000-1999 | System foundation |
| 2000-2999 | AI and intent |
| 3000-3999 | Spatial engine |
| 4000-4999 | Projection and display |
| 5000-5999 | Security and permission |
| 6000-6999 | File and data |
| 7000-7999 | Network and device |
| 8000-8999 | Update and release |
| 9000-9999 | Development and unknown errors |

Services return the code, stable message, and redacted structured `data` in JSON-RPC errors. New codes require the registry, a contract test, an owning module, and a matching documented range. Examples include `1002 CONFIGURATION_INVALID`, `4301 PROJECTION_DENIED_BY_PRIVACY_POLICY`, `5001 PERMISSION_DENIED`, and `8002 UPDATE_FAILED_ROLLED_BACK`.
