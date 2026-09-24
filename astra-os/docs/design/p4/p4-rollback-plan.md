# P4 Rollback Plan

`make p4-stop` stops only the project-owned projection service. `make p4-clean`
removes only P4 build output, test images, sockets, PIDs, and temporary caches;
it does not remove source fixtures, audit reports, or user calibration records.

On a P4 failure, the Shell calls the P1 stop/hide projection path, clears the
external output, records an audit event, and remains able to show system status
on the private Phone window. Disabling the P4 client restores P1
`ProjectionSessionService` without changing P1 schemas or policy ownership.
