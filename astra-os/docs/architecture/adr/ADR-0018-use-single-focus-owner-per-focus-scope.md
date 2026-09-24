# ADR-0018: Use One Focus Owner Per Scope

- Status: Accepted for P5 fixture-mode development
- Date: 2026-07-15

Each focus scope has exactly one primary owner and a stable traversal order.
System safety priority may preempt, while hide/detach/destroy always releases
ownership. Multiple simultaneous owners are rejected.
