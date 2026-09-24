# P1 Build Report

- Executed: 2026-07-14 (Asia/Shanghai)
- Build platform: macOS 27.0 arm64
- Compiler: Apple Clang 17.0.0
- Qt: 6.11.1
- CMake: 4.4.0
- Ninja: 1.13.2
- Build command: `make p1-build`
- Result: PASS
- Binary: `runtime/tmp/p1-build/apps/astra-shell/astra-shell`
- Architecture: Mach-O 64-bit executable arm64
- Dynamic Qt libraries: `/opt/homebrew/opt` arm64 frameworks only; no Intel Homebrew path was found.

Qt Quick, Qt Quick Controls, and Qt Quick 3D linked successfully. The release-gate run created a QRhi Metal backend for both windows, reported an Apple M1 Pro Metal device, and created CAMetalLayer surfaces. Vulkan headers remain unavailable because Vulkan is not the P1 macOS backend.
