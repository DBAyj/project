# P5 Input Routing

Platform adapters normalize mouse, keyboard, touch, and simulated gestures into
versioned input events. The router performs protected-layer, modal, z-order,
parent, then background hit testing. Invisible, disabled, transparent, or
non-interactive components are skipped. Safety overlays block propagation.
