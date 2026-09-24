# ADR-0027: Record Deferred Roadmap Modules

- Status: Accepted
- Date: 2026-09-24

## Context

The frozen module design and requirements traceability matrix assign
`astra-task-engine`, `astra-model-router`, and `astra-memory` to P2, and
`astra-file`, `astra-device`, `astra-media`, `astra-notification`,
`astra-settings`, and `astra-update` to P5.

The executed phases narrowed that scope. The P2 phase requirements delivered the
independent intent service and explicitly excluded multi-agent orchestration and
long-term memory. P5 was delivered as the Spatial UI and System Interaction
Framework. P2 and P5 both passed their release gates, yet no ADR recorded that
the modules above were not implemented, so the traceability matrix still
presents their requirements as delivered in P2 and P5.

The P5 cloud review of 2026-09-24 confirmed the implementation state:

- `FR-TASK-001`, `FR-TASK-002`, `FR-MEMORY-001`, `FR-UPDATE-001`, and
  `FR-UPDATE-002` have no implementing module. P5 task surfaces are UI state
  only; they have no ordered steps, retry, or compensation.
- `FR-AI-001` is partial: the intent service uses a deterministic local model
  adapter only, with no model router and no cloud route.
- `FR-CONFIG-001` is partial: the Shell and the Spatial UI service validate
  their configuration against strict schemas, but no `astra-settings` service
  owns the configuration lifecycle.
- `astra-file`, `astra-device`, `astra-media`, `astra-notification`, and
  `astra-settings` have no service process. P5 notification surfaces are UI
  components, not the `astra-notification` service.

## Decision

Record these modules and requirements as deferred and not yet scheduled. The
frozen phase assignments in the module design, the traceability matrix, and the
document verifier stay unchanged until a later ADR schedules the work.

The traceability matrix carries an implementation-status section that states,
for each affected requirement, whether it is deferred or partial and where the
partial coverage lives. A phase that claims one of these requirements must first
update that section and replace this deferral with a scheduling ADR.

## Consequences

- P2 and P5 completion evidence covers only their phase requirement documents,
  not the deferred modules listed here.
- Planning P6 must account for the deferred work: the P6 application runtime
  mediates services that do not exist yet, and `FR-TASK-*` is a prerequisite for
  task-level runtime behavior.
- Reports must not describe the deferred requirements as implemented.
- The mismatch between the frozen roadmap and the delivered state is now
  explicit; resolving it requires a scheduling decision.
