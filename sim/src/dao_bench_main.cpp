#include "metrics.h"
#include "sim_world.h"

#include <algorithm>
#include <cmath>
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
  float d_min = D.empty() ? 0.0f : D.front();
  float d_max = d_min;
  for (float v : D) {
    d_min = std::min(d_min, v);
    d_max = std::max(d_max, v);
  }

  const size_t mid = D.size() / 2;
  std::vector<float> first(D.begin(), D.begin() + static_cast<long>(mid));
  std::vector<float> second(D.begin() + static_cast<long>(mid), D.end());
  const float amp_first = dao::mean_abs(first);
  const float amp_second = dao::mean_abs(second);
  const float amp_ratio = (amp_first > 1e-6f) ? (amp_second / amp_first) : 0.0f;

  const bool pass_zc = zc >= 2;
  const bool pass_amp = amp_second > 0.01f && amp_second > 0.25f * amp_first;
  const bool pass = pass_zc && pass_amp;

  std::cout << "dao_only=1 neurons=" << world.graph().n << " seed=" << seed << " ticks=" << ticks
            << "\n";
  std::cout << "zc=" << zc << " D_min=" << d_min << " D_max=" << d_max
            << " amp_first=" << amp_first << " amp_second=" << amp_second
            << " amp_ratio=" << amp_ratio << "\n";
  std::cout << (pass ? "PASS" : "FAIL") << ": dao_isolated_oscillation "
            << (pass_zc ? "zc_ok" : "zc_fail") << " "
            << (pass_amp ? "amp_ok" : "amp_fail") << "\n";
  return pass ? 0 : 1;
}
