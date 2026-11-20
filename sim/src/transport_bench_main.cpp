#include "metrics.h"
#include "sim_world.h"

#include <iostream>
#include <string>

int main(int argc, char** argv) {
  uint64_t seed = 42;
  int ticks = 4000;
  if (argc >= 2) seed = static_cast<uint64_t>(std::stoull(argv[1]));
  if (argc >= 3) ticks = std::stoi(argv[2]);

  constexpr float kRatioMin = 1.25f;  // aligned transport should beat misaligned

  dao::SimConfig cfg;
  cfg.seed = seed;
  cfg.sigma_control = true;
  dao::SimWorld world(cfg);
  world.reset_transport();
  for (int t = 0; t < ticks; ++t) world.step(1);

  const auto& tr = world.transport();
  const float ratio = dao::transport_alternating_ratio(tr);
  const bool pass = ratio >= kRatioMin && (tr.yang_L_to_R + tr.yin_R_to_L) > 100;

  std::cout << "transport seed=" << seed << " ticks=" << ticks << "\n";
  std::cout << "yang_L_to_R=" << tr.yang_L_to_R << " yin_R_to_L=" << tr.yin_R_to_L
            << " yang_R_to_L=" << tr.yang_R_to_L << " yin_L_to_R=" << tr.yin_L_to_R << "\n";
  std::cout << (pass ? "PASS" : "FAIL") << ": lr_alternating_ratio " << ratio
            << " (threshold " << kRatioMin << ") margin=" << (ratio - kRatioMin) << "\n";
  return pass ? 0 : 1;
}
