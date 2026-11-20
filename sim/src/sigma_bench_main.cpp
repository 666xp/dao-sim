#include "bench_format.h"
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
  int ticks = 5000;
  int warmup = 1000;
  if (argc >= 2) seed = static_cast<uint64_t>(std::stoull(argv[1]));
  if (argc >= 3) ticks = std::stoi(argv[2]);
  if (argc >= 4) warmup = std::stoi(argv[3]);
  if (ticks < warmup + 200) ticks = warmup + 200;

  constexpr int kZcMin = 2;
  constexpr float kAmpFloor = 0.01f;
  constexpr float kAmpRatioMin = 0.25f;
  constexpr float kBreathCorr = 0.08f;
  constexpr float kBreathAxis = 0.05f;
  constexpr int kBreathAxisCount = 4;
  constexpr float kBreathMeanFallback = 0.05f;
  constexpr float kComfortMargin = 0.05f;
  constexpr float kGMin = 0.5f;
  constexpr float kGMax = 2.0f;

  dao::SimConfig cfg;
  cfg.seed = seed;
  cfg.dt_ms = 1.0f;
  cfg.fake_telemetry = false;
  cfg.dao_only = false;
  cfg.sigma_control = true;

  dao::SimWorld world(cfg);
  std::vector<float> D;
  std::vector<float> rate_ema[6];
  float ema[6] = {0, 0, 0, 0, 0, 0};
  const float alpha = 0.05f;
  float g_min = 1e9f, g_max = -1e9f;

  D.reserve(static_cast<size_t>(ticks - warmup));
  for (int i = 0; i < 6; ++i) rate_ema[i].reserve(static_cast<size_t>(ticks - warmup));

  for (int t = 0; t < ticks; ++t) {
    world.step(1);
    const auto s = world.sample();
    for (int i = 0; i < 6; ++i) ema[i] = ema_update(ema[i], s.rates[i], alpha);
    if (t < warmup) continue;
    D.push_back(s.D);
    for (int i = 0; i < 6; ++i) rate_ema[i].push_back(ema[i]);
    g_min = std::min(g_min, s.g);
    g_max = std::max(g_max, s.g);
  }

  const int zc = dao::count_zero_crossings(D);
  const size_t mid = D.size() / 2;
  std::vector<float> first(D.begin(), D.begin() + static_cast<long>(mid));
  std::vector<float> second(D.begin() + static_cast<long>(mid), D.end());
  const float amp_first = dao::mean_abs(first);
  const float amp_second = dao::mean_abs(second);
  const float amp_ratio = (amp_first > 1e-6f) ? (amp_second / amp_first) : 0.0f;

  float max_abs_corr = 0.0f;
  std::vector<float> mean_rate(D.size(), 0.0f);
  int n_axis = 0;
  for (size_t i = 0; i < D.size(); ++i) {
    float m = 0.0f;
    for (int k = 0; k < 6; ++k) m += rate_ema[k][i];
    mean_rate[i] = m / 6.0f;
  }
  for (int k = 0; k < 6; ++k) {
    const float c = dao::pearson(D, rate_ema[k]);
    max_abs_corr = std::max(max_abs_corr, std::fabs(c));
    if (std::fabs(c) >= kBreathAxis) ++n_axis;
  }
  const float corr_mean = dao::pearson(D, mean_rate);
  const bool pass_d = zc >= kZcMin && amp_second > kAmpFloor && amp_second > kAmpRatioMin * amp_first;
  const bool pass_breath = max_abs_corr >= kBreathCorr || std::fabs(corr_mean) >= kBreathCorr ||
                           (n_axis >= kBreathAxisCount && corr_mean > kBreathMeanFallback);
  const bool pass_g = g_min >= kGMin && g_max <= kGMax;
  const float breath_metric = std::max(max_abs_corr, std::fabs(corr_mean));
  const float margin = breath_metric - kBreathCorr;
  const char* tag = (!pass_breath) ? "below_gate" : (margin >= kComfortMargin ? "comfortable" : "thin");

  dao::bench_note("fields=1 sigma_control=1 seed=" + std::to_string(seed));
  dao::bench_note(std::string("breath_margin_tag=") + tag);
  dao::bench_gate(pass_d, "dao_D_stable_with_sigma", amp_ratio, kAmpRatioMin);
  dao::bench_gate(pass_breath, "field_modulation_with_sigma", breath_metric, kBreathCorr);
  dao::bench_gate(pass_g, "sigma_g_bounded", g_min, kGMin);
  dao::bench_gate(pass_d && pass_breath && pass_g, "sigma_slow_control",
                  (pass_d && pass_breath && pass_g) ? 1 : 0, 1);
  return (pass_d && pass_breath && pass_g) ? 0 : 1;
}
