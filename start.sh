#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")" && pwd)"
ENGINE="${ROOT}/sim/build/sim_engine"
PIDS=()

cleanup() {
  for p in "${PIDS[@]:-}"; do
    kill "$p" 2>/dev/null || true
  done
}
trap cleanup EXIT INT TERM

if [[ ! -x "$ENGINE" ]]; then
  echo "missing $ENGINE — build with: cmake -S sim -B sim/build && cmake --build sim/build -j"
  exit 1
fi

"$ENGINE" "$@" &
PIDS+=($!)

npm --prefix "$ROOT/web" run dev &
PIDS+=($!)

echo "engine ws://127.0.0.1:3848  web http://127.0.0.1:3847"
wait
