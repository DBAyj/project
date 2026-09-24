#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
rm -rf "$root/runtime/tmp/p1-build"
