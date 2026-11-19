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

}  // namespace dao
