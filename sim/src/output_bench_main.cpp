#include "metrics.h"
#include "sim_world.h"

#include <algorithm>
#include <cmath>
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
  // crude uniqueness over window
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

  const uint64_t seeds[] = {42, 7, 99};
  SeedStats stats[3];
  for (int i = 0; i < 3; ++i) stats[i] = run_seed(seeds[i], ticks, warmup);

  constexpr float kOnesMin = 0.02f;
  constexpr float kOnesMax = 0.85f;
  constexpr float kOnesSpreadMax = 0.25f;  // stats similar across seeds
  constexpr float kHammingMin = 0.03f;     // sequences differ

  float ones_min = stats[0].ones, ones_max = stats[0].ones;
  for (int i = 0; i < 3; ++i) {
    ones_min = std::min(ones_min, stats[i].ones);
    ones_max = std::max(ones_max, stats[i].ones);
  }
  const float ones_spread = ones_max - ones_min;

  float ham01 = dao::hamming_fraction(stats[0].bits, stats[1].bits);
  float ham02 = dao::hamming_fraction(stats[0].bits, stats[2].bits);
  float ham12 = dao::hamming_fraction(stats[1].bits, stats[2].bits);
  const float ham_mean = (ham01 + ham02 + ham12) / 3.0f;

  bool pass_nonzero = true;
  bool pass_ones_band = true;
  for (int i = 0; i < 3; ++i) {
    if (stats[i].nonzero_ticks < 10) pass_nonzero = false;
    if (stats[i].ones < kOnesMin || stats[i].ones > kOnesMax) pass_ones_band = false;
  }
  const bool pass_similar = ones_spread <= kOnesSpreadMax;
  const bool pass_diverse = ham_mean >= kHammingMin;
  const bool pass = pass_nonzero && pass_ones_band && pass_similar && pass_diverse;

  std::cout << "output_bits ticks=" << ticks << " warmup=" << warmup << "\n";
  for (int i = 0; i < 3; ++i) {
    std::cout << "seed=" << stats[i].seed << " ones=" << stats[i].ones
              << " nonzero_ticks=" << stats[i].nonzero_ticks << " unique=" << stats[i].unique
              << " tail=";
    const size_t n = stats[i].bits.size();
    for (size_t k = (n > 8 ? n - 8 : 0); k < n; ++k) {
      // print 12-bit
      for (int b = 11; b >= 0; --b) std::cout << ((stats[i].bits[k] >> b) & 1);
      std::cout << (k + 1 < n ? "," : "");
    }
    std::cout << "\n";
  }
  std::cout << "hamming 42vs7=" << ham01 << " 42vs99=" << ham02 << " 7vs99=" << ham12
            << " mean=" << ham_mean << "\n";
  std::cout << "ones_spread=" << ones_spread << " (max " << kOnesSpreadMax << ")\n";
  std::cout << (pass_nonzero ? "PASS" : "FAIL") << ": bitstream_nonzero\n";
  std::cout << (pass_ones_band ? "PASS" : "FAIL") << ": ones_density_band " << ones_min << ".."
            << ones_max << " ([" << kOnesMin << "," << kOnesMax << "])\n";
  std::cout << (pass_similar ? "PASS" : "FAIL") << ": ones_stats_similar spread=" << ones_spread
            << "\n";
  std::cout << (pass_diverse ? "PASS" : "FAIL") << ": sequences_differ hamming_mean=" << ham_mean
            << " (threshold " << kHammingMin << ")\n";
  std::cout << (pass ? "PASS" : "FAIL") << ": output_nonpreset\n";
  return pass ? 0 : 1;
}
