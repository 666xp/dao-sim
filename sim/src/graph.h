#pragma once

#include <cstdint>
#include <vector>

namespace dao {

enum class Region : uint8_t {
  LeftSediment,
  RightSediment,
  DaoYang,
  DaoYin,
  East,
  West,
  South,
  North,
  Up,
  Down,
  Output
};

struct Edge {
  uint32_t src = 0;
  uint32_t dst = 0;
  float weight = 0.0f;
  uint64_t delay_ticks = 1;
  int8_t sign = 1; // +1 excitatory, -1 inhibitory
};

struct Graph {
  uint32_t n = 0;
  std::vector<Region> region;
  std::vector<std::vector<Edge>> out_edges;
};

Graph build_default_graph(uint64_t seed);

bool has_left_right_direct_edge(const Graph& g);
bool sediment_has_non_dao_neighbor(const Graph& g);
bool all_cross_paths_via_dao(const Graph& g);
uint32_t count_isolated(const Graph& g);

bool is_dao_region(Region r);
bool is_sediment_region(Region r);
bool is_cardinal_region(Region r);

const char* region_name(Region r);

}  // namespace dao
