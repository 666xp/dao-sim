#include <catch2/catch_test_macros.hpp>

#include "criticality.h"

TEST_CASE("sigma pool excludes dao and output", "[criticality]") {
  REQUIRE(dao::region_in_sigma_pool(dao::Region::East));
  REQUIRE(dao::region_in_sigma_pool(dao::Region::LeftSediment));
  REQUIRE_FALSE(dao::region_in_sigma_pool(dao::Region::DaoYang));
  REQUIRE_FALSE(dao::region_in_sigma_pool(dao::Region::DaoYin));
  REQUIRE_FALSE(dao::region_in_sigma_pool(dao::Region::Output));
}

TEST_CASE("slow g control moves toward band", "[criticality]") {
  dao::CriticalityController c;
  c.g = 1.0f;
  c.sigma = 0.5f;
  c.window_spikes_parent = 100;
  c.window_spikes_child = 50;  // will set sigma near 0.5 then ema
  // Force low sigma then control.
  c.sigma = 0.5f;
  const float g0 = c.g;
  for (int i = 0; i < 20; ++i) {
    c.window_spikes_parent = 10;
    c.window_spikes_child = 5;
    dao::criticality_end_window_and_control(c, 1000.0f);  // 1s windows
  }
  REQUIRE(c.g > g0);
}
