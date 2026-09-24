# 18 Update and Rollback Design

The target design supports A/B system partitions, signed update packages, compatibility checks, atomic installation, configuration and data migration, automatic rollback, manual rollback, emergency recovery, and complete update auditing. This phase implements none of those operations.

The frozen flow is: discover or import package; validate signature and integrity; check hardware and version compatibility; back up configuration and critical state; write the inactive partition; reboot into verification; run health checks; promote the version. A boot or health failure selects the previous partition, restores compatible configuration, writes audit evidence, and surfaces an operations action. See [update flow](diagrams/astra-update-flow.mmd).
