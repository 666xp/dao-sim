#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace dao {

float hamming_fraction(const std::vector<uint16_t>& a, const std::vector<uint16_t>& b);
int edit_distance_u16(const std::vector<uint16_t>& a, const std::vector<uint16_t>& b);
float mean_abs(const std::vector<float>& x);
int count_zero_crossings(const std::vector<float>& x);
float pearson(const std::vector<float>& a, const std::vector<float>& b);

// Left↔Right alternating transport via dao (phase-conditioned edge firings).
struct TransportCounts {
  uint64_t yang_L_to_R = 0;  // D>0: Left→Dao or Dao→Right
  uint64_t yin_R_to_L = 0;   // D<0: Right→Dao or Dao→Left
  uint64_t yang_R_to_L = 0;  // wrong-way during yang
  uint64_t yin_L_to_R = 0;   // wrong-way during yin
};

// (aligned) / (misaligned + 1). >>1 means alternating preference.
float transport_alternating_ratio(const TransportCounts& c);

float ones_density_u16(const std::vector<uint16_t>& bits, int width = 12);
float mean_global_rate(const std::vector<float>& rates_per_tick);

}  // namespace dao
