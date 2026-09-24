# AstraOS P1 Preflight Audit

- Audit time: 2026-07-14 11:19:01 CST
- Project root: `/Users/apple/CodexProjects/astra-os`
- Starting branch: `develop`
- Starting commit: `c27c70a test: validate IPC authorization schema`
- Starting worktree: clean
- P0/P0.5 baseline: documentation verifier passed at the starting commit

## Host

| Check | Result |
| --- | --- |
| CPU architecture | `arm64` |
| Rosetta translation | inactive (`sysctl.proc_translated=0`) |
| Root filesystem free space | 134 GiB |
| Homebrew | 6.0.10 at `/opt/homebrew` |
| Xcode Command Line Tools | `/Library/Developer/CommandLineTools` |
| Apple Clang | 17.0.0 |
| CMake | 4.4.0 |
| Ninja | 1.13.2 |
| Python | 3.12.13 |
| Qt 6 | not installed at preflight |

## Result

The macOS host is native Apple Silicon with sufficient storage. Qt 6 is a required P1 build dependency and is absent; no P1 source code will be configured or compiled until the official Homebrew `qt` package is installed and its QML/Quick/Quick3D modules are verified. Docker is not required for this Qt desktop simulator and will not be used.

## Installation Attempt

An official `brew install qt` attempt started at 2026-07-14 11:20 CST and downloaded the `qt 6.11.1` bottles, but stopped while linking the `fontconfig 2.18.2` dependency. Homebrew could not replace `/opt/homebrew/opt/fontconfig` because it is a non-empty directory.

Read-only inspection found one existing link at `/opt/homebrew/opt/fontconfig/lib/libfontconfig.1.dylib` pointing into the Codex runtime cache. Replacing it with Homebrew's managed `fontconfig` link could affect that pre-existing Codex runtime path. No existing link, cache, user file, or Homebrew directory was deleted or overwritten. The full installer output is retained in the ignored runtime log `runtime/logs/p1-homebrew-qt-install.log`.

## Resolved Toolchain

After explicit authorization, only the named runtime links for `fontconfig`, `cairo`, `little-cms2`, and `expat` were replaced. Homebrew then installed `qt 6.11.1`, including `qtbase`, `qtdeclarative`, and `qtquick3d`. `qtpaths --qt-version` returned `6.11.1`, and `qmake --version` returned Qt 6.11.1 from `/opt/homebrew/lib`. The P1 Qt toolchain gate is now `PASS`.
