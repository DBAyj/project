# P2 Security Report

Security preprocessing runs after request Schema validation and before both intent engines. The service rejects prompt-injection signals with 2201, restricted control/repetition input with 2202, oversized input with 2203, unsupported locale with 2008, and other malformed requests with 2009.

The redaction suite covers phone, email, identity-card-like, payment-card-like, API-key, bearer-token, and password-field patterns, including values adjacent to Chinese text. Results and audit records contain redacted forms and no tested secret plaintext. Sensitive requests are never cached.

Unix Socket mode is `0600`. JSON-RPC and HTTP/WebSocket requests require the project runtime capability token. Wrong capability returns 5001; malformed JSON returns JSON-RPC `-32700`; confirmation expiry and replay return 2102. The token exists only as a `0600` runtime state file and is removed by `make p2-stop`.

The detector is a bounded P2 signal rather than a complete prompt-injection defense. Projection privacy remains enforced by the independent P1 policy service after intent analysis.

Security result: `PASSED` with no sensitive-data finding.
