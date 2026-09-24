# AstraOS macOS Disk Analysis

- Analysis time: 2026-07-14 07:48:37 CST
- Root filesystem: `/dev/disk3s3s1`
- Root filesystem capacity: 460 GiB (`df -h /`)
- Root filesystem available: 138 GiB (`df -h /`)
- APFS container capacity: 494.4 GB (`diskutil info /`)
- APFS container free space: 148.5 GB (`diskutil info /`)
- Required available space: at least 40 GB
- Recommended available space: at least 80 GB
- Result: `PASS` (available root space is above the 80 GB recommended threshold)

## Observed Major Directories

| Path | Observed size |
| --- | ---: |
| `/Users/apple` | 180G |
| `/Users/apple/Downloads` | 58G |
| `/Users/apple/Library` | 52G |
| `/Users/apple/Parallels` | 11G |
| `/Users/apple/Virtual Machines.localized` | 40K |

`/Users/apple` includes the listed subdirectories, so these sizes are not additive.

## Action Taken

No files, caches, virtual machines, Docker data, or downloads were deleted by Codex. This recheck confirms that the storage prerequisite is now met; no package installation or build was performed as part of the recheck.
