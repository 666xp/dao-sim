#include "dao_ode.h"

#include <algorithm>
#include <cmath>

namespace dao {

void dao_ode_step(DaoOde& ode, float dt_ms, float coupling) {
  // Van der Pol: x' = y; y' = mu*(1-x^2)*y - omega^2*x + coupling
  const float x = ode.x;
  const float y = ode.y;
  const float dx = y;
  const float dy = ode.mu * (1.0f - x * x) * y - ode.omega * ode.omega * x + coupling;
  ode.x = x + dx * dt_ms;
  ode.y = y + dy * dt_ms;
  // Soft clamp to avoid numerical blow-up.
  if (ode.x > 5.0f) ode.x = 5.0f;
  if (ode.x < -5.0f) ode.x = -5.0f;
  if (ode.y > 10.0f) ode.y = 10.0f;
  if (ode.y < -10.0f) ode.y = -10.0f;
}

float dao_ode_phase(const DaoOde& ode) {
  const float n = std::sqrt(ode.x * ode.x + ode.y * ode.y) + 1e-6f;
  return std::clamp(ode.x / n, -1.0f, 1.0f);
}

}  // namespace dao
