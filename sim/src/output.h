#pragma once

#include "graph.h"

#include <cstdint>
#include <vector>

namespace dao {

// Fixed-order bitization of Output region spikes -> uint16 (low 12 bits).
uint16_t encode_output_bits(const Graph& g, const std::vector<char>& spiked);

std::vector<uint32_t> output_neuron_ids(const Graph& g);

}  // namespace dao
