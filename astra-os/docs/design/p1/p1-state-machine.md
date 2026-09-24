# P1 Projection State Machine

States are `IDLE`, `STARTING`, `ACTIVE`, `PAUSED`, `STOPPING`, `DENIED`, and `ERROR`.

Allowed transitions: `IDLE->STARTING`, `STARTING->ACTIVE|DENIED|ERROR`, `ACTIVE->PAUSED|STOPPING`, `PAUSED->ACTIVE|STOPPING`, `STOPPING->IDLE`, `DENIED->IDLE`, and `ERROR->IDLE`. Every other transition returns error 4004, preserves state, writes an audit event, and never exposes a retained projection scene.

`ProjectionPolicyService` evaluates privacy before the session service can enter `STARTING`. `AUTHORIZED_PERSON` checks `authorization.simulated_authorized`; `PRIVATE_SCREEN_ONLY` and `NO_PROJECTION` return a denied policy decision, clear any projection content when required, and keep the projection model at `IDLE`. The QML layer receives only the resulting model state and cannot evaluate privacy itself.
