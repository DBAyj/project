# P2 Rollback Plan

1. Run the project-scoped P2 stop script; it terminates only PIDs recorded under `runtime/state` after checking their command path.
2. Disable the Intent Service client and restore the P1 `IntentSimulator` execution path through the versioned configuration switch.
3. Retain `runtime/audit` and P2 service logs; do not delete audit evidence.
4. Remove only the stale project socket after verifying its owning PID is absent.
5. Run `make p2-clean` to delete only P2 test output and regenerable runtime state; it preserves source, configuration, logs, audit records, Git metadata, and other projects.
6. Restore versioned P2 configuration from Git or use P1 safe defaults.
7. Revert P2 commits on the feature branch without rewriting P1 history.
8. Run `make p1-test` and `make p1-verify` and launch/stop P1 once.
9. Verify stop projection works with the Intent Service disabled and that start/privacy expansion remain denied in fallback mode.

Rollback does not delete the P1 tag, user files, other projects, Docker data, virtual machines, or audit logs.
