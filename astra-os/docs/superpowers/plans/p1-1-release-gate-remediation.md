# P1.1 Release-Gate Remediation Plan

## Scope

This plan remediates only the six findings in `docs/reports/p1-release-gate-report.md`. It does not create P2 code, change `VERSION`, rewrite history, or alter P1 user-facing scope.

| Item | Files | Failing-test-first scope | Verification | Rollback | Commit |
| --- | --- | --- | --- | --- | --- |
| Policy boundary | `apps/astra-shell/src/services/ProjectionPolicyService.*`, projection service/controller tests | allow/deny matrix and clear-content decision | CTest policy target | revert one commit | `refactor: isolate projection policy decisions` |
| Error registry | `libraries/astra-common/include/astra/common/ErrorCodeRegistry.h`, `libraries/astra-common/src/ErrorCodeRegistry.cpp`, `protocols/error-codes.yaml` | lookup, uniqueness, unknown fallback, exported JSON | CTest registry target and registry consistency | revert one commit | `feat: add centralized error code registry` |
| Trace and audit | `libraries/astra-common/include/astra/common/OperationContext.h`, audit service, audit schema and tests | required fields, shared trace, session reuse, schema validation | CTest plus audit-schema command | revert one commit | `feat: propagate operation trace context across P1` |
| Runtime configuration | configuration/system-state/controller/UI tests and Schema adapter | valid/invalid configuration results and Phone warning state | config-runtime command | revert one commit | `feat: enforce runtime configuration schema validation` |
| QML/accessibility | QML controls, `scripts/check_p1_qml_warnings.py`, QML tests | accessible metadata and warning-free diagnostic run | accessibility command | revert one commit | `fix: close P1 QML and accessibility warnings` |
| Graphics/windows | application diagnostics, graphics/window verification scripts and tests | actual graphics API, window state, interaction state | metal and window-interaction commands | revert one commit | `test: add independent Metal backend verification`; `test: add visible dual-window interaction evidence` |

## Finalization

Run the complete `make p1-release-gate` pipeline, update evidence reports from its output, commit only remediation code/tests/docs, merge the feature branch into `develop`, rerun the gate, then create `astra-os-p1-v0.1.0-alpha.1`. Do not create a P2 branch.
