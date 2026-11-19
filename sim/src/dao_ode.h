#pragma once

namespace dao {

// Stuart-Landau / Van der Pol style reference oscillator (panel envelope only).
struct DaoOde {
  float x = 0.1f;
  float y = 0.0f;
  float mu = 1.0f;     // negative damping strength
  float omega = 0.08f; // rad per ms approx
};

void dao_ode_step(DaoOde& ode, float dt_ms, float coupling);
float dao_ode_phase(const DaoOde& ode); // [-1,1] style D

}  // namespace dao
