#pragma once

#include "criticality.h"
#include "dao_ode.h"
#include "dao_snn.h"
#include "event_queue.h"
#include "graph.h"
#include "neuron.h"

#include <atomic>
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
  bool dao_only = false;        // phase-2: isolate half-center (no fields/sigma)
  bool sigma_control = true;    // phase-2 step3: slow g control; false for fields-only bench
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

  // Scheme A: WS thread sets this after paused step; sim thread alone publishes.
  void request_publish() { publish_requested_.store(true, std::memory_order_release); }
  bool consume_publish_request() {
    return publish_requested_.exchange(false, std::memory_order_acquire);
  }

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
  std::atomic<bool> publish_requested_{false};
  std::vector<uint16_t> recent_bits_;
  // Pending forced pulses for ablation C
  std::vector<uint32_t> forced_pulse_targets_;
};

}  // namespace dao
