# AstraOS

AstraOS is an AI-centric mobile spatial operating-system project. It coordinates private phone content, public projection content, spatial interaction, policy enforcement, and complete operational auditing. macOS is used only as a native Apple Silicon development host; the product target is ARM64 Linux LTS with Wayland and a Vulkan or OpenGL ES graphics stack.

## Baseline

The repository contains the frozen P0/P0.5 architecture, P1 dual-window Qt simulator, P2 AI Intent Center, P3 Spatial Perception and Scene Engine, the formal `astra-os-p4-v0.4.0-alpha.1` Projection Runtime baseline, and the active P5 Spatial UI framework. P5 integrates the independent services through authenticated local protocols while preserving P1 safe fallback and P4 final privacy enforcement.

| Entry point | Purpose |
| --- | --- |
| [System architecture](docs/architecture/02-system-architecture.md) | Frozen layers, boundaries, and flows |
| [PRD](docs/product/AstraOS-PRD-v1.0.md) | Product scope and MVP validation loop |
| [SRS](docs/requirements/AstraOS-SRS-v1.0.md) | Numbered functional and non-functional requirements |
| [HLD](docs/design/AstraOS-HLD-v1.0.md) | Deployment, process, communication, and platform design |
| [Protocol specification](docs/protocols/AstraOS-Protocol-Specification-v1.0.md) | JSON-RPC, WebSocket, and Unix socket rules |
| [Document verification report](docs/reports/p0-p05-document-verification.md) | Generated baseline validation evidence |

## Repository Commands

```sh
make docs-verify
make p1-release-gate
make p2-verify
make p3-verify
make p4-release-gate
make p5-release-gate
```

`make docs-verify` validates required documents, terminology, schemas, Mermaid source structure, internal links, requirement coverage, and the error-code registry. `make p1-release-gate` runs formatting and static checks, build, CTest, Schema contracts, QML tests, runtime configuration and audit checks, accessibility diagnostics, Metal verification, visible-window interaction evidence, and the frozen-document verifier. It creates only project-local runtime evidence.

P4 is a fixture-only projection renderer on this branch. `make p4-release-gate`
validates its local Unix-socket service, Shell client with P1 fallback,
project-generated fixtures, RHI/Metal runtime evidence, visual baselines,
privacy/safe-clear behavior, and measured performance/stability.

`make p5-release-gate` runs P1/P4 regression, all P5 native, contract, QML,
integration, accessibility, security, performance, stability, actual Metal
window/screenshot checks, and a project-owned run/stop cycle. P5 uses an
explicit loopback development HTTP endpoint plus capability-protected Unix
sockets; production transport remains local authenticated IPC. P5 merge and
tag eligibility requires current P1-P4 regressions plus the real P3-P4-P5
service-chain acceptance test.
