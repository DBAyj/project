# 19 Platform Abstraction

Portable modules depend on `astra-hal` interfaces and a platform-path interface, never host frameworks. The macOS development adapter may use native file locations, Metal through Qt's renderer, and host window integration. The Linux target adapter supplies Wayland, Vulkan or OpenGL ES, Linux device nodes, and target filesystem locations.

The shared contract covers paths, display targets, projectors, cameras, sensors, AI acceleration, time, secure storage, process supervision, and network-interface enumeration. All filesystem paths are constructed by an adapter. C++ and Rust targets must compile for ARM64 Linux; Python services must avoid macOS-only modules; QML must use Qt APIs available on Linux.
