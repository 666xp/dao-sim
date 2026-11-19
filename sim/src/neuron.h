#pragma once

#include <cstdint>

namespace dao {

enum class NeuronRole : uint8_t { Regular, DaoYang, DaoYin, Output };

struct Neuron {
  float v = -65.0f;
  float u = -13.0f;
  float a = 0.02f;
  float b = 0.2f;
  float c = -65.0f;
  float d = 8.0f;
  NeuronRole role = NeuronRole::Regular;
};

void configure_regular(Neuron& n);
void configure_dao_yang(Neuron& n);
void configure_dao_yin(Neuron& n);
void configure_output(Neuron& n);

void step_neuron(Neuron& n, float I, float dt_ms, bool* spiked);

}  // namespace dao
