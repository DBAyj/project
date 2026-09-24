# P2 Confirmation Policy

Privacy expansion, clearing external content, stopping an active presentation, switching to authorized-person mode, and projection operations below 0.85 confidence require confirmation. A pending confirmation binds a UUID to request, action, trace, redacted result, creation time, 30-second expiry, and one-use state.

Acceptance verifies identity, expiry, action, and unused state before returning the executable intent. Rejection and expiry are audited and cannot execute. Replayed or mismatched confirmations fail with a registered error.
