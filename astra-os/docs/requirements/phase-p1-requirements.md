# Phase P1 Requirements

## Scope

P1 delivers a single Qt application process with independent Phone Display and Projection Display windows. C++ controllers, models, and services own all behavior; QML only presents declared state and emits user intent.

## Requirements

| ID | Requirement | Acceptance |
| --- | --- | --- |
| P1-001 | Start Phone Display and Projection Display together. | Independent titled windows appear at constrained default sizes. |
| P1-002 | Parse listed Chinese/English commands without an AI model. | A typed intent, request ID, confidence, and safe `unknown` result are produced. |
| P1-003 | Apply privacy policy in C++. | `PUBLIC`/`ROOM_ONLY` allow; restricted levels clear or retain IDLE projection state. |
| P1-004 | Manage projection states. | Only documented transitions occur; illegal transitions return 4004 without mutation. |
| P1-005 | Present a Qt Quick 3D demo model. | Active session supports rotate, zoom, reset, pause, resume, and fullscreen. |
| P1-006 | Keep private phone details out of Projection Display. | Denied/restricted sessions expose only a safe empty state. |
| P1-007 | Write structured audit JSONL. | User action, policy result, and session change produce parseable redacted records. |
| P1-008 | Load strict configuration with safe fallback. | Invalid/missing config shows warning, records audit, and uses safe defaults. |

## Exclusions

No model inference, network AI, camera, gesture recognition, tracking, real projection hardware, Android compatibility, Linux image, or update implementation belongs to P1.
