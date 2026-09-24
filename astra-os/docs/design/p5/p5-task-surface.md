# P5 Task Surface

Task surfaces render structured fixture or future P2 results: title, summary,
state, intent, confidence, execution policy, privacy, progress, confirmations,
cancel, and error. P5 never performs inference. Missing P2 uses deterministic
fixtures and is reported as an unavailable optional adapter.

`spatial_ui.task.create` is the only P2-style fixture ingestion boundary. It
creates both the managed `TaskSurface` and its `TASK_CARD` component, while
`spatial_ui.task.update` advances state/progress. Both transitions emit the
dedicated task audit events and never execute AI inference.

Task surfaces share their identifier with the managed `TASK_CARD`. Removing the
component removes the corresponding surface, and `spatial_ui.reset` clears both
registries so a restored or newly created task cannot collide with stale state.
