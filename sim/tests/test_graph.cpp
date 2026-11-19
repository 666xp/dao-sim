#include <catch2/catch_test_macros.hpp>

#include "graph.h"

TEST_CASE("default graph has no isolated neurons", "[graph]") {
  auto g = dao::build_default_graph(7);
  REQUIRE(dao::count_isolated(g) == 0);
}

TEST_CASE("no left-right direct edges", "[graph]") {
  auto g = dao::build_default_graph(11);
  REQUIRE_FALSE(dao::has_left_right_direct_edge(g));
}

TEST_CASE("sediment only neighbors dao", "[graph]") {
  auto g = dao::build_default_graph(13);
  REQUIRE_FALSE(dao::sediment_has_non_dao_neighbor(g));
}

TEST_CASE("cross sediment paths go via dao", "[graph]") {
  auto g = dao::build_default_graph(17);
  REQUIRE(dao::all_cross_paths_via_dao(g));
}
