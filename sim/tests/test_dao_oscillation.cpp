#include <catch2/catch_test_macros.hpp>

#include "metrics.h"
#include "sim_world.h"

TEST_CASE("dao D continues zero-crossing without decay", "[dao]") {
  dao::SimConfig cfg;
  cfg.seed = 42;
  cfg.dt_ms = 1.0f;
  cfg.fake_telemetry = false;
  cfg.dao_only = true;  // isolate half-center; no fields / sigma / Roche

  std::vector<float> D;
  D.reserve(2000);
  dao::SimWorld w2(cfg);
  for (int i = 0; i < 2000; ++i) {
    w2.step(1);
    D.push_back(w2.dao_state().D);
  }

  const int zc = dao::count_zero_crossings(D);
  REQUIRE(zc >= 2);

  // Amplitude in second half should not collapse vs first half.
  const size_t mid = D.size() / 2;
  std::vector<float> first(D.begin(), D.begin() + static_cast<long>(mid));
  std::vector<float> second(D.begin() + static_cast<long>(mid), D.end());
  const float a1 = dao::mean_abs(first);
  const float a2 = dao::mean_abs(second);
  REQUIRE(a2 > 0.01f);
  REQUIRE(a2 > 0.25f * a1);
}
