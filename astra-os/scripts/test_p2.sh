#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
mode="${1:-all}"
"$root/scripts/configure_p2.sh" >/dev/null
mkdir -p "$root/runtime/tmp/p2-tests"
run_python_group() {
  local group="$1"
  "$root/.venv/bin/python" -u -m unittest discover \
    -s "$root/services/astra-intent-service/tests/$group" -p 'test_*.py' -v 2>&1 \
    | tee "$root/runtime/tmp/p2-tests/$group.log"
}
case "$mode" in
  unit) run_python_group unit ;;
  contract)
    run_python_group contract
    "$root/.venv/bin/python" "$root/tests/contract/test_p2_schemas.py" -v 2>&1 \
      | tee "$root/runtime/tmp/p2-tests/schema-contract.log"
    ;;
  integration) run_python_group integration ;;
  security) run_python_group security ;;
  performance)
    run_python_group performance
    ;;
  all)
    run_python_group unit
    run_python_group contract
    "$root/.venv/bin/python" "$root/tests/contract/test_p2_schemas.py" -v 2>&1 \
      | tee "$root/runtime/tmp/p2-tests/schema-contract.log"
    run_python_group integration
    run_python_group security
    "$root/scripts/build_p1.sh" >/dev/null
    ctest --test-dir "$root/runtime/tmp/p1-build" --output-on-failure 2>&1 \
      | tee "$root/runtime/tmp/p2-tests/ctest.log"
    QT_QPA_PLATFORM=offscreen QT_QUICK_CONTROLS_STYLE=Basic \
      "$(brew --prefix qt)/bin/qmltestrunner" -input "$root/apps/astra-shell/tests/qml" -silent 2>&1 \
      | tee "$root/runtime/tmp/p2-tests/qml.log"
    ;;
  *) echo "Unknown P2 test group: $mode" >&2; exit 2 ;;
esac
