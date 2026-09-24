# P5 Component Model

Components contain identity, type, parent/children, normalized bounds,
transform, z-order, opacity, enabled/visible/focusable/interactive flags,
privacy level, accessibility semantics, timestamps, and lifecycle. System-only
types are rejected for non-system principals. Illegal lifecycle transitions
return 5107, retain state, and append an audit event.

Service-level audit expands successful creation into `component_created`,
`component_attached`, and `component_shown`, and successful removal into
`component_removed` and `component_destroyed`. Each entry retains the trace,
request, actor, privacy level, component ID, result, and error code.
