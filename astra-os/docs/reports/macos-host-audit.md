# AstraOS macOS Host Audit

- Audit time: 2026-07-14 07:41:03 CST
- macOS name: macOS
- macOS version: 27.0
- System build: 26A5378j
- Kernel: Darwin 27.0.0 (`RELEASE_ARM64_T6000`)
- Device model: MacBook Pro (`MacBookPro18,3`)
- Chip: Apple M1 Pro
- CPU architecture: arm64
- Physical CPU cores: 10 (8 performance, 2 efficiency)
- Logical CPU cores: 10
- Memory: 32 GB (34,359,738,368 bytes)
- Current user: apple (uid 501)
- Current shell: /bin/zsh
- Current working directory: /Users/apple/CodexProjects/astra-os
- Root filesystem: `/dev/disk3s3s1`, APFS, internal SSD
- Root filesystem capacity: 460 GiB reported by `df -h /` (APFS container: 494.4 GB)
- Root filesystem free space: 24 GiB reported by `df -h /` (APFS container free: 25.7 GB)
- Display: built-in Liquid Retina XDR, 3024 x 1964 Retina
- Graphics processor: Apple M1 Pro GPU, 16 cores, Metal 4 support
- Rosetta status: not translated (`sysctl.proc_translated=0`; both `uname -m` and `arch` returned `arm64`)

## Result

The Codex process is running natively on Apple Silicon. No Rosetta or Intel-process blocker was detected.

The storage prerequisite is not met: usable root-partition capacity is below the required 40 GB minimum. Environment installation and build validation have not been started.
