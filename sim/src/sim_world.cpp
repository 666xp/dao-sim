#include "sim_world.h"

#include "output.h"
#include "roche.h"

#include <algorithm>
#include <cmath>

namespace dao {
namespace {

Region cardinal_by_index(int i) {
  switch (i) {
    case 0: return Region::East;
    case 1: return Region::West;
    case 2: return Region::South;
    case 3: return Region::North;
    case 4: return Region::Up;
    default: return Region::Down;
  }
}

}  // namespace

SimWorld::SimWorld(SimConfig cfg) : cfg_(cfg), rng_(cfg.seed) { rebuild(); }

void SimWorld::rebuild() {
  graph_ = cfg_.dao_only ? build_dao_only_graph(cfg_.seed) : build_default_graph(cfg_.seed);
  neurons_ = build_neurons(graph_);
  I_syn_.assign(graph_.n, 0.0f);
  spiked_.assign(graph_.n, 0);
  while (!queue_.empty()) queue_.pop();
  crit_ = CriticalityController{};
  ode_ = DaoOde{};
  dao_state_ = DaoSnnState{};
  tick_ = 0;
  recent_bits_.clear();
  forced_pulse_targets_.clear();
  rng_.seed(cfg_.seed);
}

void SimWorld::start() { running_ = true; }
void SimWorld::pause() { running_ = false; }

void SimWorld::set_seed(uint64_t seed) {
  cfg_.seed = seed;
  rebuild();
}

void SimWorld::set_ablation(const AblationFlags& f) { ablation_ = f; }

void SimWorld::set_g(float g) { crit_.g = std::clamp(g, 0.2f, 3.0f); }

void SimWorld::perturb(Region group, uint32_t pulses) {
  std::vector<uint32_t> ids;
  for (uint32_t i = 0; i < graph_.n; ++i) {
    if (graph_.region[i] == group) ids.push_back(i);
  }
  if (ids.empty()) return;
  for (uint32_t p = 0; p < pulses; ++p) {
    forced_pulse_targets_.push_back(ids[rng_() % ids.size()]);
  }
}

float SimWorld::region_rate(Region r) const {
  uint32_t n = 0, s = 0;
  for (uint32_t i = 0; i < graph_.n; ++i) {
    if (graph_.region[i] != r) continue;
    ++n;
    if (spiked_[i]) ++s;
  }
  return n ? static_cast<float>(s) / static_cast<float>(n) : 0.0f;
}

void SimWorld::step(uint64_t n_ticks) {
  for (uint64_t i = 0; i < n_ticks; ++i) step_one();
}

void SimWorld::step_one() {
  if (cfg_.fake_telemetry) {
    ++tick_;
    dao_state_.D = std::sin(0.01f * static_cast<float>(tick_));
    crit_.sigma = 0.9f + 0.2f * (0.5f + 0.5f * std::sin(0.003f * static_cast<float>(tick_)));
    spiked_.assign(graph_.n, 0);
    // Light fake spikes on output for bitstream motion.
    for (uint32_t i = 0; i < graph_.n; ++i) {
      if (graph_.region[i] == Region::Output && ((tick_ + i) % 17ull) == 0) spiked_[i] = 1;
    }
    const uint16_t bits = encode_output_bits(graph_, spiked_);
    recent_bits_.push_back(bits);
    if (recent_bits_.size() > 4096) {
      recent_bits_.erase(recent_bits_.begin(), recent_bits_.begin() + 1024);
    }
    dao_ode_step(ode_, cfg_.dt_ms, 0.0f);
    return;
  }

  std::fill(I_syn_.begin(), I_syn_.end(), 0.0f);
  std::fill(spiked_.begin(), spiked_.end(), 0);

  // Deliver due synaptic events.
  std::vector<SpikeEvent> due;
  queue_.advance_to(tick_, due);
  for (const auto& ev : due) {
    if (ev.target < graph_.n) I_syn_[ev.target] += ev.amp;
  }

  // Forced pulses (ablation C).
  if (!forced_pulse_targets_.empty()) {
    uint32_t t = forced_pulse_targets_.back();
    forced_pulse_targets_.pop_back();
    if (t < graph_.n) I_syn_[t] += 15.0f;
  }

  std::uniform_real_distribution<float> noise_dist(-1.0f, 1.0f);

  // Drive currents: dao half-center bias + phase modulation onto fields.
  const float Dprev = dao_state_.D;
  for (uint32_t i = 0; i < graph_.n; ++i) {
    float I = I_syn_[i];
    const Region r = graph_.region[i];

    // Intrinsic bias for dao half-center self-excitation.
    if (r == Region::DaoYang) I += 4.5f + 2.0f * std::max(0.0f, -Dprev);
    if (r == Region::DaoYin) I += 4.5f + 2.0f * std::max(0.0f, Dprev);

    // Phase alternating drive onto sediments (tonic + D-signed modulation).
    if (r == Region::LeftSediment) I += 3.0f + 4.0f * Dprev * crit_.g;
    if (r == Region::RightSediment) I += 3.0f - 4.0f * Dprev * crit_.g;

    // Cardinals: tonic bias so they fire, plus D-phase modulation ("breathe with D").
    if (is_cardinal_region(r)) I += 3.2f + 4.5f * Dprev * crit_.g;

    if (ablation_.noise) I += 0.8f * noise_dist(rng_);

    // Ablation A: disable adaptation current update for dao roles by freezing u.
    float u_before = neurons_[i].u;
    bool spiked = false;
    step_neuron(neurons_[i], I, cfg_.dt_ms, &spiked);
    if (!ablation_.dao_adaptation &&
        (neurons_[i].role == NeuronRole::DaoYang || neurons_[i].role == NeuronRole::DaoYin)) {
      neurons_[i].u = u_before;
    }
    spiked_[i] = spiked ? 1 : 0;

    if (spiked) {
      uint32_t child_pool = 0;
      for (const auto& e : graph_.out_edges[i]) {
        float amp = e.weight * static_cast<float>(e.sign);
        // Apply g only on excitatory synapses in sigma pool and dao->pool projections.
        const bool src_pool = region_in_sigma_pool(graph_.region[i]);
        const bool dst_pool = region_in_sigma_pool(graph_.region[e.dst]);
        const bool dao_to_pool = is_dao_region(graph_.region[i]) && dst_pool;
        if (e.sign > 0 && (src_pool || dao_to_pool)) amp *= crit_.g;

        // Roche shaping for dao-related edges (skipped in dao_only isolation).
        if (!cfg_.dao_only &&
            (is_dao_region(graph_.region[i]) || is_dao_region(graph_.region[e.dst]))) {
          amp *= roche_weight(graph_.region[i], graph_.region[e.dst], i % 64, e.dst % 64, 64, 64);
        }

        SpikeEvent se;
        se.delivery_tick = tick_ + e.delay_ticks;
        se.target = e.dst;
        se.amp = amp * 8.0f;
        queue_.push(se);
        if (dst_pool) ++child_pool;
      }
      criticality_observe_spike_pair(crit_, region_in_sigma_pool(graph_.region[i]), child_pool);
    }
  }

  dao_state_ = measure_dao(graph_, spiked_, 0.08f, &dao_state_);
  // ODE envelope: weak coupling from SNN D, not pointwise lock.
  dao_ode_step(ode_, cfg_.dt_ms, 0.02f * dao_state_.D);

  // Sigma slow control (skip for dao_only / fields-only benches).
  if (cfg_.sigma_control && !cfg_.dao_only && tick_ % 50ull == 0) {
    criticality_end_window_and_control(crit_, 50.0f * cfg_.dt_ms);
  }

  const uint16_t bits = encode_output_bits(graph_, spiked_);
  recent_bits_.push_back(bits);
  if (recent_bits_.size() > 4096) {
    recent_bits_.erase(recent_bits_.begin(), recent_bits_.begin() + 1024);
  }

  ++tick_;
}

TelemetrySample SimWorld::sample() const {
  TelemetrySample t;
  t.tick = tick_;
  t.D = dao_state_.D;
  t.D_ode = dao_ode_phase(ode_);
  t.sigma = crit_.sigma;
  t.g = crit_.g;
  for (int i = 0; i < 6; ++i) t.rates[i] = region_rate(cardinal_by_index(i));
  t.out_bits = recent_bits_.empty() ? 0 : recent_bits_.back();
  return t;
}

}  // namespace dao
