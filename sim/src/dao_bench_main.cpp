#include "bench_format.h"
#include "metrics.h"
#include "sim_world.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char** argv) {
  uint64_t seed = 42;
  int ticks = 2000;
  if (argc >= 2) seed = static_cast<uint64_t>(std::stoull(argv[1]));
  if (argc >= 3) ticks = std::stoi(argv[2]);
  if (ticks < 10) ticks = 10;

  constexpr int kZcMin = 2;
  constexpr float kAmpFloor = 0.01f;
  constexpr float kAmpRatioMin = 0.25f;

  dao::SimConfig cfg;
  cfg.seed = seed;
  cfg.dt_ms = 1.0f;
  cfg.fake_telemetry = false;
  cfg.dao_only = true;

  dao::SimWorld world(cfg);
  std::vector<float> D;
  D.reserve(static_cast<size_t>(ticks));
  for (int i = 0; i < ticks; ++i) {
    world.step(1);
    D.push_back(world.dao_state().D);
  }

  const int zc = dao::count_zero_crossings(D);
  const size_t mid = D.size() / 2;
  std::vector<float> first(D.begin(), D.begin() + static_cast<long>(mid));
  std::vector<float> second(D.begin() + static_cast<long>(mid), D.end());
  const float amp_first = dao::mean_abs(first);
  const float amp_second = dao::mean_abs(second);
  const float amp_ratio = (amp_first > 1e-6f) ? (amp_second / amp_first) : 0.0f;

  const bool pass_zc = zc >= kZcMin;
  const bool pass_amp = amp_second > kAmpFloor && amp_second > kAmpRatioMin * amp_first;

  dao::bench_note("dao_only=1 neurons=" + std::to_string(world.graph().n) + " seed=" +
                  std::to_string(seed) + " ticks=" + std::to_string(ticks));
  dao::bench_gate(pass_zc, "dao_zc", zc, kZcMin);
  dao::bench_gate(pass_amp, "dao_amp_ratio", amp_ratio, kAmpRatioMin);
  dao::bench_gate(pass_zc && pass_amp, "dao_isolated_oscillation",
                  (pass_zc && pass_amp) ? 1 : 0, 1);
  return (pass_zc && pass_amp) ? 0 : 1;
}
