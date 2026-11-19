#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

echo "=== audit: output.cpp string literals (exclude #include) ==="
rg -n '"' sim/src/output.cpp | rg -v '#include' || true

echo "=== audit: forbidden consciousness / semantic map markers in sim/src ==="
if rg -n 'conscious|意识开关|semantic_map|neuron_to_word' sim/src; then
  echo "FAIL: forbidden markers found"
  exit 1
fi
echo "PASS: no forbidden markers"

echo "=== audit: validate CLI sample ==="
./sim/build/sim_validate 42
