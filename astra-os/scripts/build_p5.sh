#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
"$root/scripts/configure_p5.sh"
cmake --build "$root/runtime/tmp/p5-build"
service_info="$(file "$root/runtime/tmp/p5-build/services/astra-spatial-ui-service/astra-spatial-ui-service")"
shell_info="$(file "$root/runtime/tmp/p5-build/apps/astra-shell/astra-shell")"
echo "$service_info"
echo "$shell_info"
if [[ "$service_info" != *arm64* || "$shell_info" != *arm64* ]]; then
  echo "P5 binaries are not arm64" >&2
  exit 1
fi
mkdir -p "$root/docs/reports"
cat > "$root/docs/reports/p5-build-report.md" <<EOF
# P5 Build Report

Baseline: \`P4_RELEASE_BASELINE_FINAL\`

Result: \`P5_BUILD_PASSED\`

The RelWithDebInfo Ninja build completed for the independent Spatial UI
service and Astra Shell on the Apple Silicon development host.

- Spatial UI Service: \`$service_info\`
- Astra Shell: \`$shell_info\`
- Product target remains ARM64 Linux LTS; macOS is the development adapter.
EOF
