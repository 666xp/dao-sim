#include "criticality.h"

#include <algorithm>

namespace dao {

bool region_in_sigma_pool(Region r) {
  // Six cardinals + left/right sediments. Exclude dao SNN and output.
  return is_sediment_region(r) || is_cardinal_region(r);
}

void criticality_observe_spike_pair(CriticalityController& c, bool parent_in_pool,
                                    uint32_t child_in_pool_count) {
  if (!parent_in_pool) return;
  c.window_spikes_parent += 1;
  c.window_spikes_child += child_in_pool_count;
}

void criticality_end_window_and_control(CriticalityController& c, float dt_ms) {
  if (c.window_spikes_parent > 0) {
    // child_* counts pool out-edges (fan-out), not delivered child spikes.
    // Normalize by a typical fan-out so sigma sits near ~1 at criticality-like wiring.
    constexpr float kFanoutRef = 3.5f;
    const float raw =
        static_cast<float>(c.window_spikes_child) / static_cast<float>(c.window_spikes_parent);
    const float inst = raw / kFanoutRef;
    c.sigma = 0.95f * c.sigma + 0.05f * inst;  // slower sigma EMA
  }
  c.window_spikes_parent = 0;
  c.window_spikes_child = 0;

  // Slow proportional pull of g when sigma leaves the band.
  float err = 0.0f;
  if (c.sigma < c.target_lo) err = c.target_lo - c.sigma;
  else if (c.sigma > c.target_hi) err = c.target_hi - c.sigma;

  const float dt_sec = dt_ms * 0.001f;
  const float alpha = std::min(1.0f, dt_sec / std::max(0.5f, c.tau_sec));
  c.g += alpha * 0.35f * err;
  // Keep g away from extremes so dao/fields are not crushed.
  c.g = std::clamp(c.g, 0.5f, 2.0f);
}

}  // namespace dao
