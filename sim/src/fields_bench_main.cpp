#include "metrics.h"
#include "sim_world.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

float ema_update(float prev, float x, float alpha) { return (1.0f - alpha) * prev + alpha * x; }

}  // namespace

int main(int argc, char** argv) {
  uint64_t seed = 42;
  int ticks = 3000;
  int warmup = 500;
  if (argc >= 2) seed = static_cast<uint64_t>(std::stoull(argv[1]));
  if (argc >= 3) ticks = std::stoi(argv[2]);
  if (argc >= 4) warmup = std::stoi(argv[3]);
  if (ticks < warmup + 100) ticks = warmup + 100;

  dao::SimConfig cfg;
  cfg.seed = seed;
  cfg.dt_ms = 1.0f;
  cfg.fake_telemetry = false;
  cfg.dao_only = false;
  cfg.sigma_control = false;  // step 2: fields + Roche only; sigma is step 3

  dao::SimWorld world(cfg);
  std::vector<float> D;
  std::vector<float> rate_ema[6];
  float ema[6] = {0, 0, 0, 0, 0, 0};
  const float alpha = 0.05f;

  D.reserve(static_cast<size_t>(ticks - warmup));
  for (int i = 0; i < 6; ++i) rate_ema[i].reserve(static_cast<size_t>(ticks - warmup));

  for (int t = 0; t < ticks; ++t) {
    world.step(1);
    const auto s = world.sample();
    for (int i = 0; i < 6; ++i) ema[i] = ema_update(ema[i], s.rates[i], alpha);
    if (t < warmup) continue;
    D.push_back(s.D);
    for (int i = 0; i < 6; ++i) rate_ema[i].push_back(ema[i]);
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

  const char* names[6] = {"E", "W", "S", "N", "Up", "Down"};
  float corr[6] = {0, 0, 0, 0, 0, 0};
  float max_abs_corr = 0.0f;
  std::vector<float> mean_rate(D.size(), 0.0f);
  for (size_t i = 0; i < D.size(); ++i) {
    float m = 0.0f;
    for (int k = 0; k < 6; ++k) m += rate_ema[k][i];
    mean_rate[i] = m / 6.0f;
  }
  for (int k = 0; k < 6; ++k) {
    corr[k] = dao::pearson(D, rate_ema[k]);
    max_abs_corr = std::max(max_abs_corr, std::fabs(corr[k]));
  }
  const float corr_mean = dao::pearson(D, mean_rate);

  const bool pass_d = zc >= 2 && amp_second > 0.01f && amp_second > 0.25f * amp_first;
  // Threshold 0.08: MSVC/Clang float paths differ; require clear but not brittle coupling.
  int n_pos = 0;
  for (int k = 0; k < 6; ++k) {
    if (std::fabs(corr[k]) >= 0.05f) ++n_pos;
  }
  const bool pass_breath =
      max_abs_corr >= 0.08f || std::fabs(corr_mean) >= 0.08f || (n_pos >= 4 && corr_mean > 0.05f);
  const bool pass = pass_d && pass_breath;

  std::cout << "fields=1 sigma_control=0 neurons=" << world.graph().n << " seed=" << seed
            << " ticks=" << ticks << " warmup=" << warmup << "\n";
  std::cout << "zc=" << zc << " D_min=" << d_min << " D_max=" << d_max
            << " amp_first=" << amp_first << " amp_second=" << amp_second
            << " amp_ratio=" << amp_ratio << "\n";
  std::cout << "corr_mean=" << corr_mean << " max_abs_corr=" << max_abs_corr;
  for (int k = 0; k < 6; ++k) std::cout << " corr_" << names[k] << "=" << corr[k];
  std::cout << "\n";
  std::cout << (pass ? "PASS" : "FAIL") << ": fields_roche_integration "
            << (pass_d ? "D_ok" : "D_fail") << " " << (pass_breath ? "breath_ok" : "breath_fail")
            << "\n";
  return pass ? 0 : 1;
}
