# P2 Security Design

The input boundary limits text to 2,000 characters, rejects control characters and more than 100 repeated characters, validates locale and protocol shape, and normalizes Unicode before engines run. Sensitive detectors recognize phone, email, identity-card-like, payment-card-like, API-key, bearer-token, and password-field patterns. Audit uses redacted text only.

The injection detector recognizes explicit attempts to bypass rules, reveal prompts/secrets, delete files, or execute terminal commands. A high-risk match returns 2201 and `REJECT`, records a security audit, and never calls engines or projection. This is a bounded defensive signal, not a claim of complete injection prevention.
