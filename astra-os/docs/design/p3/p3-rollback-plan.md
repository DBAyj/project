# P3 Rollback Plan

1. Run `make p3-stop`; verify the spatial service, Shell, and P2 service labels/PIDs are absent.
2. Stop and destroy the active `FrameSource`; camera and video handles must be released.
3. Remove only the P3 Socket and regenerable state through `make p3-clean`.
4. Preserve `runtime/spatial/calibration`, spatial audit logs, application logs, and all user data.
5. Disable P3 mode and start the accepted P2 run path; fixed P1 projection remains available.
6. Restore P3 configuration from Git or select simulation without deleting profiles.
7. Revert P3 commits on the P3 branch without rewriting P2 history.
8. Run `make p1-test`, `make p1-verify`, `make p2-test`, and `make p2-verify`.
9. Verify P2 stop/hide/status fallback and P1 privacy denial remain functional.
10. Do not delete source, Git metadata, audit evidence, other projects, Docker data, or virtual machines.
