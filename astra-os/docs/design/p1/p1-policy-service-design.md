# P1 Projection Policy Service

`ProjectionPolicyService` is the P1 adapter for the frozen `astra-policy` boundary. It owns the allow or deny decision for every simulator projection start. Its request contains the privacy level, simulated authorization result, requested action, and current projection state. Its decision returns `allowed`, a registered error code, a user-safe reason, whether external content must be cleared, and whether the result requires audit.

`PUBLIC` and `ROOM_ONLY` allow projection. `AUTHORIZED_PERSON` allows only when `authorization.simulated_authorized` is true; otherwise it returns 4302. `PRIVATE_SCREEN_ONLY` denies external projection. `NO_PROJECTION` denies with 4301 and requires clearing any retained projection content. Unknown privacy input is denied.

`ShellController` coordinates a decision and writes its audit result. `ProjectionSessionService` receives only approved starts and owns state transitions and session IDs. QML renders the resulting state and cannot reimplement policy. The matrix is covered by `astra-shell-projection-policy-tests`.
