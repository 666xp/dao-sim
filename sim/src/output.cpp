#include "output.h"

namespace dao {

std::vector<uint32_t> output_neuron_ids(const Graph& g) {
  std::vector<uint32_t> ids;
  for (uint32_t i = 0; i < g.n; ++i) {
    if (g.region[i] == Region::Output) ids.push_back(i);
  }
  return ids;
}

uint16_t encode_output_bits(const Graph& g, const std::vector<char>& spiked) {
  uint16_t bits = 0;
  uint32_t bit = 0;
  for (uint32_t i = 0; i < g.n && bit < 12; ++i) {
    if (g.region[i] != Region::Output) continue;
    if (i < spiked.size() && spiked[i]) {
      bits = static_cast<uint16_t>(bits | (1u << bit));
    }
    ++bit;
  }
  // High 4 bits remain 0 by construction (only 12 bits used).
  return static_cast<uint16_t>(bits & 0x0FFFu);
}

}  // namespace dao
