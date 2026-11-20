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
  c.tau_sec = 5.0f;
  // Force low normalized sigma then control (child/parent/fanout_ref << 1).
  c.sigma = 0.5f;
  const float g0 = c.g;
  for (int i = 0; i < 40; ++i) {
    c.window_spikes_parent = 10;
    c.window_spikes_child = 5;  // raw 0.5 / 3.5 => low inst
    dao::criticality_end_window_and_control(c, 1000.0f);  // 1s windows
  }
  REQUIRE(c.g > g0);
  REQUIRE(c.g <= 2.0f);
  REQUIRE(c.g >= 0.5f);
}
