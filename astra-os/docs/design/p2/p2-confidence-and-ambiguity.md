# P2 Confidence and Ambiguity

Configured thresholds are auto-execute `0.85`, confirmation `0.65`, reject `0.45`, and ambiguity delta `0.10`. High-confidence, low-risk, complete requests may auto-execute. Risky or mid-confidence requests require confirmation. Conflicting candidates, missing slots, conflicting context/privacy, simultaneous start/stop, and incompatible phone/projection clauses ask for clarification. Scores below reject threshold are rejected.

Clarification returns a short user-facing question and bounded options. Internal rule IDs, model implementation details, prompts, and sensitive values are not included.
