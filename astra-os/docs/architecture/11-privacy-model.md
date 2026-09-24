# 11 Privacy Model

Every displayable object has a privacy level. The UI may render a policy result but may not decide it; `astra-policy` returns the final decision and `astra-projection` enforces it.

| Level | Meaning | External-projection behavior |
| --- | --- | --- |
| `PUBLIC` | Safe for a public surface. | Allowed after ordinary device and capability checks. |
| `ROOM_ONLY` | Suitable only for the current physical room. | Allowed with a visible room-level privacy indicator. |
| `AUTHORIZED_PERSON` | Restricted to verified people. | Allowed only after authorized-person validation. |
| `PRIVATE_SCREEN_ONLY` | Private to the primary phone display. | Never emitted to an external display. |
| `NO_PROJECTION` | Never externally visible. | Always denied, including mirrored output. |

Classification accompanies `SceneObject`, `ProjectionSession`, media metadata, and all policy requests. Cloud-model routing additionally evaluates data classification before egress. A `4301` denial is audit mandatory.
