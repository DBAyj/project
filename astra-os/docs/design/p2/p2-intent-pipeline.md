# P2 Intent Pipeline

The fixed order is Schema validation, length limits, Unicode and whitespace normalization, sensitive-data detection/redaction, prompt-injection detection, rule evaluation, deterministic model evaluation, candidate merge, slot extraction, context completion, confidence correction, ambiguity detection, risk classification, confirmation policy, result creation, audit emission, and response.

Security stages cannot be bypassed by an adapter. Engine errors become candidate-source health evidence and invoke documented fallback. Cache lookup occurs only after normalization/security and uses normalized text, locale, relevant context, rules version, and model version. Cached entries never contain raw sensitive input or confirmation tokens.
