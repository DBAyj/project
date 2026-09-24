# P5 Interaction State Machine

The runtime input boundary owns one `InteractionSession` per active component.
Pointer and simulated gesture events enter through `InputRouter`, move through
Pressed/Dragging/Resizing/Rotating/Scaling, and either commit or cancel with a
bounds/transform rollback. Window move and resize RPCs use the same state
machine and return explicit interaction evidence for audit.

Interactions use explicit IDLE, HOVERED, PRESSED, SELECTED, DRAGGING,
RESIZING, ROTATING, SCALING, DISABLED, CANCELLED, and ERROR states. Controllers
store the start transform, apply bounded updates, then commit or roll back.
Every accepted or rejected transition is auditable.

`GESTURE_SCALE` and `GESTURE_ROTATE` carry transform deltas in the explicit
`interaction_value` field. Scale fixtures accept `0.1..10.0`; rotation fixtures
accept `-360..360` degrees. Pointer coordinates remain coordinates and are not
reused as transform values. An active pointer, touch, or simulated-gesture
source captures its component until release or cancellation, including when the
release position is outside the component bounds.
