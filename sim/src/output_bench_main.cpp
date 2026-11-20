#include "bench_format.h"
#include "metrics.h"
#include "sim_world.h"

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

namespace {

struct SeedStats {
  uint64_t seed = 0;
  float ones = 0.0f;
  int unique = 0;
  int nonzero_ticks = 0;
  std::vector<uint16_t> bits;
};

SeedStats run_seed(uint64_t seed, int ticks, int warmup) {
  dao::SimConfig cfg;
  cfg.seed = seed;
  cfg.sigma_control = true;
  dao::SimWorld world(cfg);
  SeedStats s;
  s.seed = seed;
  s.bits.reserve(static_cast<size_t>(ticks - warmup));
  for (int t = 0; t < ticks; ++t) {
    world.step(1);
    if (t < warmup) continue;
    const uint16_t b = world.sample().out_bits;
    s.bits.push_back(b);
    if (b) ++s.nonzero_ticks;
  }
  s.ones = dao::ones_density_u16(s.bits, 12);
  std::vector<int> seen(4096, 0);
  for (uint16_t b : s.bits) {
    if (!seen[b & 0x0FFF]) {
      seen[b & 0x0FFF] = 1;
      ++s.unique;
    }
  }
  return s;
}

}  // namespace

int main(int argc, char** argv) {
  int ticks = 4000;
  int warmup = 500;
  if (argc >= 2) ticks = std::stoi(argv[1]);
  if (argc >= 3) warmup = std::stoi(argv[2]);

  constexpr float kOnesMin = 0.02f;
  constexpr float kOnesMax = 0.85f;
  constexpr float kOnesSpreadMax = 0.25f;
  constexpr float kHammingMin = 0.03f;

  const uint64_t seeds[] = {42, 7, 99};
  SeedStats stats[3];
  for (int i = 0; i < 3; ++i) stats[i] = run_seed(seeds[i], ticks, warmup);

  float ones_min = stats[0].ones, ones_max = stats[0].ones;
  for (int i = 0; i < 3; ++i) {
    ones_min = std::min(ones_min, stats[i].ones);
    ones_max = std::max(ones_max, stats[i].ones);
  }
  const float ones_spread = ones_max - ones_min;
  const float ham_mean = (dao::hamming_fraction(stats[0].bits, stats[1].bits) +
                          dao::hamming_fraction(stats[0].bits, stats[2].bits) +
                          dao::hamming_fraction(stats[1].bits, stats[2].bits)) /
                         3.0f;

  bool pass_nonzero = true;
  bool pass_ones_band = true;
  for (int i = 0; i < 3; ++i) {
    if (stats[i].nonzero_ticks < 10) pass_nonzero = false;
    if (stats[i].ones < kOnesMin || stats[i].ones > kOnesMax) pass_ones_band = false;
  }
  const bool pass_similar = ones_spread <= kOnesSpreadMax;
  const bool pass_diverse = ham_mean >= kHammingMin;
  const bool pass = pass_nonzero && pass_ones_band && pass_similar && pass_diverse;

  for (int i = 0; i < 3; ++i) {
    dao::bench_note("seed=" + std::to_string(stats[i].seed) + " ones=" + std::to_string(stats[i].ones) +
                    " unique=" + std::to_string(stats[i].unique) +
                    " nonzero_ticks=" + std::to_string(stats[i].nonzero_ticks));
  }
  dao::bench_note(
      "nonpreset_group: ones_density≈0.027 + hamming≈0.05 must be read together under sparsity");
  dao::bench_gate(pass_nonzero, "bitstream_nonzero",
                  static_cast<double>(stats[0].nonzero_ticks + stats[1].nonzero_ticks +
                                      stats[2].nonzero_ticks),
                  30);
  dao::bench_gate(pass_ones_band, "ones_density_band", ones_min, kOnesMin);
  dao::bench_gate(pass_similar, "ones_stats_similar", ones_spread, kOnesSpreadMax);
  dao::bench_gate(pass_diverse, "sequences_differ", ham_mean, kHammingMin);
  dao::bench_gate(pass, "output_nonpreset", pass ? 1 : 0, 1);
  return pass ? 0 : 1;
}
