#include "metrics.h"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace dao {

float hamming_fraction(const std::vector<uint16_t>& a, const std::vector<uint16_t>& b) {
  const size_t n = std::min(a.size(), b.size());
  if (n == 0) return 0.0f;
  size_t diff = 0;
  for (size_t i = 0; i < n; ++i) {
    uint16_t x = static_cast<uint16_t>(a[i] ^ b[i]);
    // popcount
    while (x) {
      diff += x & 1u;
      x = static_cast<uint16_t>(x >> 1);
    }
  }
  return static_cast<float>(diff) / static_cast<float>(n * 12);
}

int edit_distance_u16(const std::vector<uint16_t>& a, const std::vector<uint16_t>& b) {
  const size_t n = a.size();
  const size_t m = b.size();
  std::vector<int> prev(m + 1), cur(m + 1);
  for (size_t j = 0; j <= m; ++j) prev[j] = static_cast<int>(j);
  for (size_t i = 1; i <= n; ++i) {
    cur[0] = static_cast<int>(i);
    for (size_t j = 1; j <= m; ++j) {
      const int cost = a[i - 1] == b[j - 1] ? 0 : 1;
      cur[j] = std::min({prev[j] + 1, cur[j - 1] + 1, prev[j - 1] + cost});
    }
    prev.swap(cur);
  }
  return prev[m];
}

float mean_abs(const std::vector<float>& x) {
  if (x.empty()) return 0.0f;
  double s = 0.0;
  for (float v : x) s += std::fabs(v);
  return static_cast<float>(s / static_cast<double>(x.size()));
}

int count_zero_crossings(const std::vector<float>& x) {
  int c = 0;
  for (size_t i = 1; i < x.size(); ++i) {
    if ((x[i - 1] < 0.0f && x[i] >= 0.0f) || (x[i - 1] >= 0.0f && x[i] < 0.0f)) ++c;
  }
  return c;
}

float pearson(const std::vector<float>& a, const std::vector<float>& b) {
  const size_t n = std::min(a.size(), b.size());
  if (n < 2) return 0.0f;
  double ma = 0, mb = 0;
  for (size_t i = 0; i < n; ++i) {
    ma += a[i];
    mb += b[i];
  }
  ma /= static_cast<double>(n);
  mb /= static_cast<double>(n);
  double num = 0, da = 0, db = 0;
  for (size_t i = 0; i < n; ++i) {
    const double xa = a[i] - ma;
    const double xb = b[i] - mb;
    num += xa * xb;
    da += xa * xa;
    db += xb * xb;
  }
  const double den = std::sqrt(da * db) + 1e-12;
  return static_cast<float>(num / den);
}

float transport_alternating_ratio(const TransportCounts& c) {
  const double aligned = static_cast<double>(c.yang_L_to_R + c.yin_R_to_L);
  const double wrong = static_cast<double>(c.yang_R_to_L + c.yin_L_to_R);
  return static_cast<float>(aligned / (wrong + 1.0));
}

float ones_density_u16(const std::vector<uint16_t>& bits, int width) {
  if (bits.empty() || width <= 0) return 0.0f;
  uint64_t ones = 0;
  for (uint16_t b : bits) {
    uint16_t x = static_cast<uint16_t>(b & static_cast<uint16_t>((1u << width) - 1u));
    while (x) {
      ones += x & 1u;
      x = static_cast<uint16_t>(x >> 1);
    }
  }
  return static_cast<float>(ones) /
         static_cast<float>(bits.size() * static_cast<size_t>(width));
}

float mean_global_rate(const std::vector<float>& rates_per_tick) {
  return mean_abs(rates_per_tick);  // rates are non-negative; mean_abs == mean
}

}  // namespace dao
