# P5 Security Design

Capabilities default deny. System-only types cannot be created by application
principals. Input cannot cross safety overlays. Destroyed components cannot
receive input or retain focus. State snapshots are strict and fail closed.
Target loss and P4 failure hide external UI, clear cached frames, and invoke
the existing P1/P4 safe fallback.

The five runtime configuration documents are Schema-validated before use.
Missing files, parse errors, unknown keys, type/range violations, or invalid
layout values select `SAFE_DEFAULTS` and disable external projection. Security
tests cover this fail-closed branch together with capability denial, final P4
privacy filtering, target-loss clearing, and audit parsing.

Fixture capability credentials are derived into distinct
`spatial_ui.read`, `spatial_ui.write`, `spatial_ui.input`, and
`spatial_ui.system` tokens before dispatch. A token from one method class is
rejected by every other class. The P4 service applies the same method-scoped
derivation to render, control, health, and output-read operations. P4 also
validates each policy decision as an HMAC binding over subject, privacy level,
and allow result using the service credential; clients cannot mint a valid
decision from public fields.

The fixture configuration owns the single deterministic public subject. All
other caller declarations are capped at `PRIVATE_SCREEN_ONLY`, and component
updates may only preserve or strengthen the existing classification.
