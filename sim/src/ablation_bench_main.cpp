#include "bench_format.h"
#include "metrics.h"
#include "sim_world.h"

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

namespace {

struct RunResult {
  std::vector<float> D;
  std::vector<uint16_t> bits;
  std::vector<float> global_rates;
  dao::TransportCounts transport{};
  float mean_global = 0.0f;
  int zc = 0;
  float alt_ratio = 0.0f;
};

RunResult run_world(uint64_t seed, int ticks, int warmup, dao::AblationFlags abl, bool perturb_c) {
  dao::SimConfig cfg;
  cfg.seed = seed;
  cfg.sigma_control = true;
  dao::SimWorld world(cfg);
  world.set_ablation(abl);
  world.reset_transport();
  RunResult r;
  for (int t = 0; t < ticks; ++t) {
    if (perturb_c && t == warmup + 200) world.perturb(dao::Region::East, 50);
    world.step(1);
    if (t < warmup) continue;
    const auto s = world.sample();
    r.D.push_back(s.D);
    r.bits.push_back(s.out_bits);
    r.global_rates.push_back(s.global_rate);
  }
  r.transport = world.transport();
  r.zc = dao::count_zero_crossings(r.D);
  r.alt_ratio = dao::transport_alternating_ratio(r.transport);
  r.mean_global = dao::mean_global_rate(r.global_rates);
  return r;
}

}  // namespace

int main(int argc, char** argv) {
  uint64_t seed = 42;
  int ticks = 5000;
  int warmup = 800;
  if (argc >= 2) seed = static_cast<uint64_t>(std::stoull(argv[1]));
  if (argc >= 3) ticks = std::stoi(argv[2]);

  dao::AblationFlags base{};
  base.dao_adaptation = true;
  base.noise = true;
  const RunResult ctrl = run_world(seed, ticks, warmup, base, false);

  dao::AblationFlags a = base;
  a.dao_adaptation = false;
  const RunResult abl_a = run_world(seed, ticks, warmup, a, false);

  dao::AblationFlags b = base;
  b.noise = false;
  const RunResult b42 = run_world(42, ticks, warmup, b, false);
  const RunResult b7 = run_world(7, ticks, warmup, b, false);
  const RunResult c42 = run_world(42, ticks, warmup, base, false);
  const RunResult c7 = run_world(7, ticks, warmup, base, false);
  const float ham_noise_on = dao::hamming_fraction(c42.bits, c7.bits);
  const float ham_noise_off = dao::hamming_fraction(b42.bits, b7.bits);

  const RunResult abl_c = run_world(seed, ticks, warmup, base, true);
  const size_t win = std::min<size_t>(400, std::min(ctrl.bits.size(), abl_c.bits.size()));
  std::vector<uint16_t> ctrl_tail(ctrl.bits.end() - static_cast<long>(win), ctrl.bits.end());
  std::vector<uint16_t> c_tail(abl_c.bits.end() - static_cast<long>(win), abl_c.bits.end());
  const int ed = dao::edit_distance_u16(ctrl_tail, c_tail);
  const float ed_frac = win ? static_cast<float>(ed) / static_cast<float>(win) : 0.0f;

  const float global_ratio =
      (ctrl.mean_global > 1e-9f) ? (abl_a.mean_global / ctrl.mean_global) : 0.0f;

  const bool a_rhythm =
      abl_a.zc < std::max(2, ctrl.zc / 3) || abl_a.alt_ratio < 0.85f * std::max(1.0f, ctrl.alt_ratio);
  const bool a_not_global_collapse = abl_a.mean_global > 0.45f * ctrl.mean_global;
  const bool a_bits_changed = dao::hamming_fraction(ctrl.bits, abl_a.bits) > 0.02f;
  const bool pass_a = a_rhythm && a_not_global_collapse && a_bits_changed;
  const bool pass_b = ham_noise_off <= ham_noise_on * 1.02f + 0.002f;
  const bool pass_c = ed_frac > 0.05f;
  const bool pass = pass_a && pass_b && pass_c;

  dao::bench_note("ablation_A: u-adaptation off raises dao excitability; global_rate rise is expected "
                  "if bounded (see ratio)");
  dao::bench_gate(pass_a, "ablation_A_dao_adapt", abl_a.zc, static_cast<double>(std::max(2, ctrl.zc / 3)));
  dao::bench_gate(a_not_global_collapse, "ablation_A_global_rate_ratio", global_ratio, 0.45);
  dao::bench_gate(pass_b, "ablation_B_noise_convergence", ham_noise_on - ham_noise_off, 0.0);
  dao::bench_gate(pass_c, "ablation_C_east_perturb", ed_frac, 0.05);
  dao::bench_gate(pass, "phase3_ablations", pass ? 1 : 0, 1);
  return pass ? 0 : 1;
}
