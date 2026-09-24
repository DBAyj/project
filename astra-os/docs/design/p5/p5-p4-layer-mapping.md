# P5 To P4 Layer Mapping

Spatial windows and task surfaces map to APPLICATION_SURFACE; assistant cards
to AI_ASSISTANT; notifications to NOTIFICATION; development overlays to
DEBUG_OVERLAY. Privacy badges use a protected system-owned layer ordered above
ordinary content. PRIVATE_SCREEN_ONLY and NO_PROJECTION never map. P5 calls
public P4 requests only and cannot access warp, color, or offscreen passes.

Every submitted layer includes the UUID returned by `astra-policy` as
`policy_decision_id` plus `policy_subject_id`; the P4 fixture service verifies
that the decision is bound to the layer object, privacy level, and allow result.
P5 has no command-line policy bypass. The fixture adapter
draws the bounded public accessibility label into the P4 frame and exposes the
accepted labels in `projection.output.frame`. Clearing the output also clears
the label set, and private labels never enter it.
