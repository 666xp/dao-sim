#include "dao_snn.h"

#include <cmath>

namespace dao {

std::vector<Neuron> build_neurons(const Graph& g) {
  std::vector<Neuron> neurons(g.n);
  for (uint32_t i = 0; i < g.n; ++i) {
    switch (g.region[i]) {
      case Region::DaoYang:
        configure_dao_yang(neurons[i]);
        break;
      case Region::DaoYin:
        configure_dao_yin(neurons[i]);
        break;
      case Region::Output:
        configure_output(neurons[i]);
        break;
      default:
        configure_regular(neurons[i]);
        break;
    }
  }
  return neurons;
}

DaoSnnState measure_dao(const Graph& g, const std::vector<char>& spiked_this_tick, float ema_alpha,
                        DaoSnnState* prev) {
  DaoSnnState s;
  uint32_t yang_n = 0, yin_n = 0;
  for (uint32_t i = 0; i < g.n; ++i) {
    if (g.region[i] == Region::DaoYang) {
      ++yang_n;
      if (i < spiked_this_tick.size() && spiked_this_tick[i]) ++s.yang_spikes;
    } else if (g.region[i] == Region::DaoYin) {
      ++yin_n;
      if (i < spiked_this_tick.size() && spiked_this_tick[i]) ++s.yin_spikes;
    }
  }
  const float yr = yang_n ? static_cast<float>(s.yang_spikes) / static_cast<float>(yang_n) : 0.0f;
  const float nr = yin_n ? static_cast<float>(s.yin_spikes) / static_cast<float>(yin_n) : 0.0f;
  if (prev) {
    s.yang_rate = (1.0f - ema_alpha) * prev->yang_rate + ema_alpha * yr;
    s.yin_rate = (1.0f - ema_alpha) * prev->yin_rate + ema_alpha * nr;
  } else {
    s.yang_rate = yr;
    s.yin_rate = nr;
  }
  const float den = s.yang_rate + s.yin_rate + 1e-4f;
  s.D = (s.yang_rate - s.yin_rate) / den;
  s.D = std::max(-1.0f, std::min(1.0f, s.D));
  return s;
}

}  // namespace dao
