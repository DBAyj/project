# P5 Spatial Window Manager

The manager owns up to the configured limit and is the only writer for window
state, target, bounds, focus scope, anchor reference, and stacking order. Move
and resize clamp to the target safe area; invalid dimensions fail with 5205.
Target loss hides external content and requests P4 safe clear. When the target
becomes available again, hidden components stay hidden: availability is not
consent to project again, so the caller must show each component explicitly.

A closed window stays queryable with state `CLOSED`, but it no longer counts
toward the window limit, is not persisted, and may be reopened with the same
identifier. `window_count` in status, metrics, and window queries counts only
windows that are not closed. Removing a component removes its windows.
