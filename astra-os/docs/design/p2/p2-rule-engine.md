# P2 Rule Engine

Rules are loaded from strict `intent-rules.yaml` and sorted by priority then stable rule ID. A rule may define exact phrases, required keyword groups, synonyms, simple `{slot}` templates, locale, required/default slots, confidence, enabled state, and negation terms. Exact matches outrank template and keyword matches.

Negated action phrases cannot produce an automatic positive action. Unknown fields, duplicate IDs, unsupported intents, invalid confidence, and invalid slot names reject the whole rules snapshot. Configuration changes increment the rules version and invalidate cached results.
