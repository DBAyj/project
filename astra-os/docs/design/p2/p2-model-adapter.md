# P2 Model Adapter

`IntentModelAdapter.predict(text, locale, context)` returns deterministic `IntentCandidate` values. P2 implements `DeterministicIntentModel` with normalized n-gram token overlap and curated examples. It has no network path, model artifact, training loop, large framework, or mutable random state.

The adapter exposes health and version. Timeout, explicit failure injection for tests, and adapter unavailability are translated to registered errors. Rule-only fallback remains available; a future model implementation must preserve the same interface and security boundary.
