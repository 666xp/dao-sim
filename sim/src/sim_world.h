#pragma once

#include "criticality.h"
#include "dao_ode.h"
#include "dao_snn.h"
#include "event_queue.h"
#include "graph.h"
#include "neuron.h"

#include <cstdint>
#include <random>
#include <vector>

namespace dao {

struct AblationFlags {
  bool dao_adaptation = true;  // when false: freeze u updates for DaoYang/DaoYin
  bool noise = true;
};

struct SimConfig {
  uint64_t seed = 42;
  float dt_ms = 1.0f;
  bool fake_telemetry = false;  // phase-1 style sine wave
};

struct TelemetrySample {
  uint64_t tick = 0;
  float D = 0.0f;
  float D_ode = 0.0f;
  float sigma = 1.0f;
  float g = 1.0f;
  float rates[6] = {0, 0, 0, 0, 0, 0};  // E W S N Up Down
  uint16_t out_bits = 0;
};

class SimWorld {
 public:
  explicit SimWorld(SimConfig cfg);

  void start();
  void pause();
  bool running() const { return running_; }

  void step(uint64_t n_ticks);
  void set_seed(uint64_t seed);
  void set_ablation(const AblationFlags& f);
  void set_g(float g);
  void perturb(Region group, uint32_t pulses);

  TelemetrySample sample() const;
  const Graph& graph() const { return graph_; }
  const AblationFlags& ablation() const { return ablation_; }
  uint64_t tick() const { return tick_; }

  // For tests: run dao-only style drive check helpers
  const DaoSnnState& dao_state() const { return dao_state_; }
  std::vector<uint16_t> recent_bits() const { return recent_bits_; }

 private:
  void rebuild();
  void step_one();
  float region_rate(Region r) const;

  SimConfig cfg_;
  AblationFlags ablation_{};
  Graph graph_;
  std::vector<Neuron> neurons_;
  std::vector<float> I_syn_;
  std::vector<char> spiked_;
  EventQueue queue_;
  CriticalityController crit_{};
  DaoOde ode_{};
  DaoSnnState dao_state_{};
  std::mt19937_64 rng_;
  uint64_t tick_ = 0;
  bool running_ = false;
  std::vector<uint16_t> recent_bits_;
  // Pending forced pulses for ablation C
  std::vector<uint32_t> forced_pulse_targets_;
};

}  // namespace dao
