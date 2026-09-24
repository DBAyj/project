# P5 Rollback Plan

`make p5-stop` stops only project-owned P5 processes, clears the P5 socket and
Shell cache, and leaves audit logs intact. Configuration can disable the P5
component tree so the Shell returns to its P1 interface and P4 direct fixture
control. UI state may be retained if schema-valid or removed only by an
explicit P5 cleanup option. `make p5-clean` removes P5 build/temp artifacts,
then P1 and P4 regression gates verify privacy isolation and safe clear.
