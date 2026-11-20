#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")" && pwd)"
PIDS=()

cleanup() {
  for p in "${PIDS[@]:-}"; do
    kill "$p" 2>/dev/null || true
  done
}
trap cleanup EXIT INT TERM

ENGINE=""
for cand in \
  "${ROOT}/sim/build/Debug/sim_engine" \
  "${ROOT}/sim/build/Release/sim_engine" \
  "${ROOT}/sim/build/sim_engine"
do
  if [[ -x "$cand" ]]; then
    ENGINE="$cand"
    break
  fi
done

if [[ -z "$ENGINE" ]]; then
  echo "sim_engine not found. Build first:"
  echo "  cmake -S sim -B sim/build -DDAO_SIM_BUILD_ENGINE=ON -DDAO_SIM_BUILD_DYNAMICS=ON"
  echo "  cmake --build sim/build --config Debug -j"
  exit 1
fi

echo "Starting engine: $ENGINE --fake"
"$ENGINE" --fake --port 3848 &
PIDS+=($!)

npm --prefix "$ROOT/web" run dev &
PIDS+=($!)

echo "engine ws://127.0.0.1:3848  web http://127.0.0.1:3847"
echo "Phase 1: fake telemetry. Cloud smoke != acceptance; confirm curves locally."
wait
