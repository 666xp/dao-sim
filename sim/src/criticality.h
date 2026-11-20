#pragma once

#include "graph.h"

#include <cstdint>
#include <vector>

namespace dao {

struct CriticalityController {
  float g = 1.0f;           // global excitatory gain on sigma pool
  float target_lo = 0.9f;
  float target_hi = 1.1f;
  float tau_sec = 5.0f;     // slow timescale (seconds); must stay >> neuron ms scale
  float sigma = 1.0f;
  uint64_t window_spikes_parent = 0;
  uint64_t window_spikes_child = 0;
};

// Update branching-ratio estimate from parent/child spike counts in sigma pool.
void criticality_observe_spike_pair(CriticalityController& c, bool parent_in_pool,
                                    uint32_t child_in_pool_count);

void criticality_end_window_and_control(CriticalityController& c, float dt_ms);

bool region_in_sigma_pool(Region r);

}  // namespace dao
