# P5 Spatial Window Manager

The manager owns up to the configured limit and is the only writer for window
state, target, bounds, focus scope, anchor reference, and stacking order. Move
and resize clamp to the target safe area; invalid dimensions fail with 5205.
Target loss hides external content and requests P4 safe clear.
