# P5 Focus Management

Each focus scope has one owner. Focus requests validate visibility,
interactivity, priority, and scope. System and privacy confirmation priorities
may preempt; ordinary notifications do not. Hide, detach, destroy, and target
loss release focus. Stable component order defines keyboard traversal.

A critical notification records `focus_preempted` with the prior owner and
reason. Handling it records `focus_changed` with the previous and restored
owners. The RPC result carries the same reason and focus IDs, using an empty
string only for no owner and UUIDs for every real component.

Critical notifications form a restore chain. Clearing the newest notification
restores the previous critical owner; clearing an older notification rewrites
the chain so no destroyed owner can be restored later.
