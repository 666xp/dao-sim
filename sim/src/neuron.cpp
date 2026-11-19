#include "neuron.h"

namespace dao {

void configure_regular(Neuron& n) {
  // Regular spiking
  n.a = 0.02f;
  n.b = 0.2f;
  n.c = -65.0f;
  n.d = 8.0f;
  n.v = -65.0f;
  n.u = n.b * n.v;
  n.role = NeuronRole::Regular;
}

void configure_dao_yang(Neuron& n) {
  // Chattering / bursting with slower adaptation (distinct from yin).
  n.a = 0.02f;
  n.b = 0.2f;
  n.c = -50.0f;
  n.d = 2.0f;
  n.v = -55.0f;
  n.u = n.b * n.v;
  n.role = NeuronRole::DaoYang;
}

void configure_dao_yin(Neuron& n) {
  // Bursting with faster adaptation — asymmetry for half-center.
  n.a = 0.1f;
  n.b = 0.2f;
  n.c = -55.0f;
  n.d = 2.0f;
  n.v = -65.0f;
  n.u = n.b * n.v;
  n.role = NeuronRole::DaoYin;
}

void configure_output(Neuron& n) {
  configure_regular(n);
  n.role = NeuronRole::Output;
}

void step_neuron(Neuron& n, float I, float dt_ms, bool* spiked) {
  if (spiked) *spiked = false;
  // Simple Euler with substeps for stability at 1 ms.
  const int sub = 2;
  const float h = dt_ms / static_cast<float>(sub);
  for (int s = 0; s < sub; ++s) {
    const float dv = (0.04f * n.v * n.v + 5.0f * n.v + 140.0f - n.u + I) * h;
    const float du = (n.a * (n.b * n.v - n.u)) * h;
    n.v += dv;
    n.u += du;
    if (n.v >= 30.0f) {
      n.v = n.c;
      n.u += n.d;
      if (spiked) *spiked = true;
      break;
    }
  }
}

}  // namespace dao
