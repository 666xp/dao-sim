#pragma once

#include "graph.h"
#include "neuron.h"

#include <cstdint>
#include <vector>

namespace dao {

struct DaoSnnState {
  float yang_rate = 0.0f;
  float yin_rate = 0.0f;
  float D = 0.0f;  // (yang - yin) / (yang + yin + eps) in [-1,1]
  uint32_t yang_spikes = 0;
  uint32_t yin_spikes = 0;
};

// Build neuron population matching graph regions.
std::vector<Neuron> build_neurons(const Graph& g);

DaoSnnState measure_dao(const Graph& g, const std::vector<char>& spiked_this_tick,
                        float ema_alpha = 0.05f, DaoSnnState* prev = nullptr);

}  // namespace dao
