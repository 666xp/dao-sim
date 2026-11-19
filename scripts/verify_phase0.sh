#!/usr/bin/env bash
# Phase 0 only: graph / event_queue / neuron / validate.
# Does NOT build engine or phase-2 dynamics tests.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="${ROOT}/sim/build-phase0"

cmake -S "${ROOT}/sim" -B "${BUILD}" \
  -DDAO_SIM_BUILD_ENGINE=OFF \
  -DDAO_SIM_BUILD_DYNAMICS=OFF \
  -DDAO_SIM_BUILD_TESTS=ON

cmake --build "${BUILD}" -j"$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)"

ctest --test-dir "${BUILD}" --output-on-failure

VALIDATE="${BUILD}/sim_validate"
if [[ ! -x "${VALIDATE}" ]]; then
  VALIDATE="${BUILD}/Release/sim_validate"
fi
"${VALIDATE}" 42
echo "PASS: phase0_verify_complete 1"
