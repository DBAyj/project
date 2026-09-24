#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
"$root/scripts/build_p5.sh" >/dev/null
ctest --test-dir "$root/runtime/tmp/p5-build" -R '^(astra-ui-(privacy|input|focus|state-store)-tests|astra-spatial-ui-(service|runtime-command|runtime-projection|runtime-state|configuration)-tests|astra-shell-p5-spatial-ui-integration-tests|astra-shell-p5-spatial-target-chain-tests)$' --output-on-failure
"$root/.venv/bin/python" "$root/scripts/verify_p5_privacy_isolation.py"
python3 "$root/scripts/verify_p5_interaction_flow.py"
cat > "$root/docs/reports/p5-security-report.md" <<'EOF'
# P5 Security Evidence

Baseline: `P4_RELEASE_BASELINE_FINAL`

Result: `P5_SECURITY_PASSED`

Method-scoped read/write/input/system capability denial, validated-
configuration fail-closed behavior, object/privacy-bound P4 policy decisions,
safety-layer input blocking, destroyed-focus cleanup, tampered-state rejection,
policy-only state restore classification, private/no-projection layer isolation,
target-loss safe hiding, P4 final privacy mapping, audit parsing, and Socket
cleanup passed. Method capabilities were derived from the central protocol
registry by both native services and Python fixture clients.
The deterministic mouse, keyboard, and simulated-gesture flows also passed.
EOF
