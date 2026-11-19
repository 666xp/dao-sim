#include "roche.h"

#include <algorithm>
#include <cmath>

namespace dao {

float roche_weight(Region src, Region dst, uint32_t src_local, uint32_t dst_local,
                   uint32_t src_count, uint32_t dst_count) {
  auto pos = [](Region r, uint32_t local, uint32_t count) -> float {
    const float t = count > 1 ? static_cast<float>(local) / static_cast<float>(count - 1) : 0.5f;
    // Map regions onto a 1D "orbital" line: left at -1, dao near 0, right at +1, cardinals mid.
    switch (r) {
      case Region::LeftSediment: return -1.0f + 0.2f * t;
      case Region::RightSediment: return 1.0f - 0.2f * t;
      case Region::DaoYang: return -0.15f + 0.1f * t;
      case Region::DaoYin: return 0.05f + 0.1f * t;
      case Region::East: return 0.35f * t;
      case Region::West: return -0.35f * t;
      case Region::South: return 0.1f * (t - 0.5f);
      case Region::North: return 0.1f * (0.5f - t);
      case Region::Up: return 0.05f * t;
      case Region::Down: return -0.05f * t;
      case Region::Output: return 0.0f;
    }
    return 0.0f;
  };

  const float x1 = pos(src, src_local, std::max(1u, src_count));
  const float x2 = pos(dst, dst_local, std::max(1u, dst_count));
  // Effective potential shape: deep near midplane (Lagrange-like), weaker far away.
  const float mid = 0.5f * (x1 + x2);
  const float depth = std::exp(-3.0f * mid * mid);
  const float sep = std::fabs(x1 - x2);
  const float couple = 1.0f / (1.0f + sep);
  return std::clamp(0.35f + 0.65f * depth * couple, 0.05f, 1.0f);
}

}  // namespace dao
