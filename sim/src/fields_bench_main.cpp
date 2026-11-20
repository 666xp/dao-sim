#include "metrics.h"
#include "sim_world.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iomanip>
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

  // Gates (also printed so phase-4 lights / margin checks share one source of truth).
  constexpr int kZcMin = 2;
  constexpr float kAmpFloor = 0.01f;
  constexpr float kAmpRatioMin = 0.25f;
  constexpr float kBreathCorr = 0.08f;
  constexpr float kBreathAxis = 0.05f;
  constexpr int kBreathAxisCount = 4;
  constexpr float kBreathMeanFallback = 0.05f;
  constexpr float kComfortMargin = 0.05f;  // above gate → "comfortable", else "thin"

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

  const bool pass_d =
      zc >= kZcMin && amp_second > kAmpFloor && amp_second > kAmpRatioMin * amp_first;
  int n_axis = 0;
  for (int k = 0; k < 6; ++k) {
    if (std::fabs(corr[k]) >= kBreathAxis) ++n_axis;
  }
  const bool breath_by_max = max_abs_corr >= kBreathCorr;
  const bool breath_by_mean = std::fabs(corr_mean) >= kBreathCorr;
  const bool breath_by_axes = n_axis >= kBreathAxisCount && corr_mean > kBreathMeanFallback;
  const bool pass_breath = breath_by_max || breath_by_mean || breath_by_axes;
  const bool pass = pass_d && pass_breath;

  const float breath_metric = std::max(max_abs_corr, std::fabs(corr_mean));
  const float breath_margin = breath_metric - kBreathCorr;
  const char* breath_margin_tag =
      (!pass_breath) ? "below_gate" : (breath_margin >= kComfortMargin ? "comfortable" : "thin");

  std::cout << std::setprecision(6);
  std::cout << "fields=1 sigma_control=0 neurons=" << world.graph().n << " seed=" << seed
            << " ticks=" << ticks << " warmup=" << warmup << "\n";
  std::cout << "D: zc=" << zc << " (min " << kZcMin << ") D_min=" << d_min << " D_max=" << d_max
            << " amp_first=" << amp_first << " amp_second=" << amp_second
            << " amp_ratio=" << amp_ratio << " (min " << kAmpRatioMin << ")\n";
  std::cout << "breath: corr_mean=" << corr_mean << " max_abs_corr=" << max_abs_corr
            << " n_axis_ge_" << kBreathAxis << "=" << n_axis;
  for (int k = 0; k < 6; ++k) std::cout << " corr_" << names[k] << "=" << corr[k];
  std::cout << "\n";
  std::cout << (pass_d ? "PASS" : "FAIL") << ": dao_D_stable zc=" << zc
            << " amp_ratio=" << amp_ratio << " (threshold amp_ratio>=" << kAmpRatioMin
            << " amp_second>" << kAmpFloor << ")\n";
  std::cout << (pass_breath ? "PASS" : "FAIL") << ": field_modulation " << breath_metric
            << " (threshold " << kBreathCorr << ") margin=" << breath_margin << " ["
            << breath_margin_tag << "]\n";
  std::cout << (pass ? "PASS" : "FAIL") << ": fields_roche_integration "
            << (pass_d ? "D_ok" : "D_fail") << " " << (pass_breath ? "breath_ok" : "breath_fail")
            << "\n";
  return pass ? 0 : 1;
}
