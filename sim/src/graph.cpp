#include "graph.h"

#include <algorithm>
#include <queue>
#include <random>
#include <unordered_set>

namespace dao {
namespace {

struct RegionSpec {
  Region region;
  uint32_t count;
};

void add_edge(Graph& g, uint32_t src, uint32_t dst, float w, uint64_t delay, int8_t sign) {
  if (src == dst) return;
  Edge e;
  e.src = src;
  e.dst = dst;
  e.weight = w;
  e.delay_ticks = std::max<uint64_t>(1, delay);
  e.sign = sign;
  g.out_edges[src].push_back(e);
}

bool edge_exists(const Graph& g, uint32_t src, uint32_t dst) {
  for (const auto& e : g.out_edges[src]) {
    if (e.dst == dst) return true;
  }
  return false;
}

void ensure_degree_at_least_one(Graph& g, std::mt19937_64& rng, const std::vector<uint32_t>& pool) {
  if (pool.size() < 2) return;
  std::uniform_int_distribution<size_t> pick(0, pool.size() - 1);
  for (uint32_t i : pool) {
    bool has_out = !g.out_edges[i].empty();
    bool has_in = false;
    for (uint32_t j : pool) {
      if (j == i) continue;
      if (edge_exists(g, j, i)) {
        has_in = true;
        break;
      }
    }
    if (!has_out) {
      uint32_t dst = pool[pick(rng)];
      while (dst == i) dst = pool[pick(rng)];
      add_edge(g, i, dst, 0.3f, 1 + (rng() % 5), 1);
    }
    if (!has_in) {
      uint32_t src = pool[pick(rng)];
      while (src == i) src = pool[pick(rng)];
      add_edge(g, src, i, 0.3f, 1 + (rng() % 5), 1);
    }
  }
}

void wire_small_world(Graph& g, std::mt19937_64& rng, const std::vector<uint32_t>& nodes,
                      float density, float inhib_frac) {
  if (nodes.size() < 2) return;
  const size_t n = nodes.size();
  size_t target_edges = std::max<size_t>(n, static_cast<size_t>(density * n * (n - 1)));
  target_edges = std::min(target_edges, n * (n - 1));
  std::uniform_int_distribution<size_t> pick(0, n - 1);
  std::uniform_real_distribution<float> uni(0.0f, 1.0f);

  // Ring skeleton for strong connectivity tendency.
  for (size_t i = 0; i < n; ++i) {
    uint32_t a = nodes[i];
    uint32_t b = nodes[(i + 1) % n];
    if (!edge_exists(g, a, b)) {
      add_edge(g, a, b, 0.4f, 1 + (rng() % 3), 1);
    }
  }

  size_t made = 0;
  size_t guard = 0;
  while (made < target_edges && guard < target_edges * 20) {
    ++guard;
    uint32_t a = nodes[pick(rng)];
    uint32_t b = nodes[pick(rng)];
    if (a == b || edge_exists(g, a, b)) continue;
    int8_t sign = uni(rng) < inhib_frac ? -1 : 1;
    float w = 0.15f + 0.5f * uni(rng);
    uint64_t delay = 1 + (rng() % 8);
    add_edge(g, a, b, w, delay, sign);
    ++made;
  }
  ensure_degree_at_least_one(g, rng, nodes);
}

void wire_bipartite(Graph& g, std::mt19937_64& rng, const std::vector<uint32_t>& a,
                    const std::vector<uint32_t>& b, float dens, bool allow_inhibit) {
  if (a.empty() || b.empty()) return;
  std::uniform_real_distribution<float> uni(0.0f, 1.0f);
  for (uint32_t i : a) {
    for (uint32_t j : b) {
      if (uni(rng) > dens) continue;
      int8_t sign = (allow_inhibit && uni(rng) < 0.2f) ? -1 : 1;
      add_edge(g, i, j, 0.2f + 0.4f * uni(rng), 1 + (rng() % 6), sign);
      if (uni(rng) < dens) {
        add_edge(g, j, i, 0.2f + 0.4f * uni(rng), 1 + (rng() % 6),
                 (allow_inhibit && uni(rng) < 0.2f) ? -1 : 1);
      }
    }
  }
}

bool can_reach_avoiding(const Graph& g, uint32_t src, uint32_t dst,
                        const std::unordered_set<uint32_t>& forbidden) {
  if (src == dst) return true;
  std::vector<char> seen(g.n, 0);
  std::queue<uint32_t> q;
  q.push(src);
  seen[src] = 1;
  while (!q.empty()) {
    uint32_t u = q.front();
    q.pop();
    for (const auto& e : g.out_edges[u]) {
      if (forbidden.count(e.dst)) continue;
      if (seen[e.dst]) continue;
      if (e.dst == dst) return true;
      seen[e.dst] = 1;
      q.push(e.dst);
    }
  }
  return false;
}

bool can_reach(const Graph& g, uint32_t src, uint32_t dst) {
  std::unordered_set<uint32_t> none;
  return can_reach_avoiding(g, src, dst, none);
}

}  // namespace

bool is_dao_region(Region r) {
  return r == Region::DaoYang || r == Region::DaoYin;
}

bool is_sediment_region(Region r) {
  return r == Region::LeftSediment || r == Region::RightSediment;
}

bool is_cardinal_region(Region r) {
  return r == Region::East || r == Region::West || r == Region::South || r == Region::North ||
         r == Region::Up || r == Region::Down;
}

const char* region_name(Region r) {
  switch (r) {
    case Region::LeftSediment: return "LeftSediment";
    case Region::RightSediment: return "RightSediment";
    case Region::DaoYang: return "DaoYang";
    case Region::DaoYin: return "DaoYin";
    case Region::East: return "East";
    case Region::West: return "West";
    case Region::South: return "South";
    case Region::North: return "North";
    case Region::Up: return "Up";
    case Region::Down: return "Down";
    case Region::Output: return "Output";
  }
  return "Unknown";
}

Graph build_default_graph(uint64_t seed) {
  // Scale from plan: sediments 256 each, dao ~128, cardinals 6x64, output 12.
  const std::vector<RegionSpec> specs = {
      {Region::LeftSediment, 256},
      {Region::RightSediment, 256},
      {Region::DaoYang, 64},
      {Region::DaoYin, 64},
      {Region::East, 64},
      {Region::West, 64},
      {Region::South, 64},
      {Region::North, 64},
      {Region::Up, 64},
      {Region::Down, 64},
      {Region::Output, 12},
  };

  Graph g;
  uint32_t total = 0;
  for (const auto& s : specs) total += s.count;
  g.n = total;
  g.region.assign(total, Region::East);
  g.out_edges.assign(total, {});

  std::vector<std::vector<uint32_t>> pools(static_cast<size_t>(Region::Output) + 1);
  uint32_t idx = 0;
  for (const auto& s : specs) {
    for (uint32_t k = 0; k < s.count; ++k) {
      g.region[idx] = s.region;
      pools[static_cast<size_t>(s.region)].push_back(idx);
      ++idx;
    }
  }

  std::mt19937_64 rng(seed);

  // Intra-region wiring. Sediments and cardinals: small-world. Dao: denser mutual inhibition later.
  for (const auto& s : specs) {
    const auto& pool = pools[static_cast<size_t>(s.region)];
    float dens = 0.07f;
    float inhib = 0.2f;
    if (is_dao_region(s.region)) {
      dens = 0.15f;
      inhib = 0.35f;
    }
    if (s.region == Region::Output) {
      dens = 0.25f;
      inhib = 0.1f;
    }
    wire_small_world(g, rng, pool, dens, inhib);
  }

  // Dao yang/yin cross inhibition (half-center backbone).
  wire_bipartite(g, rng, pools[static_cast<size_t>(Region::DaoYang)],
                 pools[static_cast<size_t>(Region::DaoYin)], 0.25f, true);
  // Force inhibitory bias on cross dao edges.
  for (uint32_t i : pools[static_cast<size_t>(Region::DaoYang)]) {
    for (auto& e : g.out_edges[i]) {
      if (g.region[e.dst] == Region::DaoYin) e.sign = -1;
    }
  }
  for (uint32_t i : pools[static_cast<size_t>(Region::DaoYin)]) {
    for (auto& e : g.out_edges[i]) {
      if (g.region[e.dst] == Region::DaoYang) e.sign = -1;
    }
  }

  // Sediments ONLY <-> Dao (no cardinal / output / other sediment).
  wire_bipartite(g, rng, pools[static_cast<size_t>(Region::LeftSediment)],
                 pools[static_cast<size_t>(Region::DaoYang)], 0.05f, false);
  wire_bipartite(g, rng, pools[static_cast<size_t>(Region::LeftSediment)],
                 pools[static_cast<size_t>(Region::DaoYin)], 0.05f, false);
  wire_bipartite(g, rng, pools[static_cast<size_t>(Region::RightSediment)],
                 pools[static_cast<size_t>(Region::DaoYang)], 0.05f, false);
  wire_bipartite(g, rng, pools[static_cast<size_t>(Region::RightSediment)],
                 pools[static_cast<size_t>(Region::DaoYin)], 0.05f, false);

  // Cardinals <-> Dao bidirectional.
  const Region cards[] = {Region::East, Region::West, Region::South,
                          Region::North, Region::Up, Region::Down};
  for (Region c : cards) {
    wire_bipartite(g, rng, pools[static_cast<size_t>(c)],
                   pools[static_cast<size_t>(Region::DaoYang)], 0.05f, true);
    wire_bipartite(g, rng, pools[static_cast<size_t>(c)],
                   pools[static_cast<size_t>(Region::DaoYin)], 0.05f, true);
  }

  // Sparse inter-cardinal links.
  for (size_t i = 0; i < 6; ++i) {
    for (size_t j = i + 1; j < 6; ++j) {
      wire_bipartite(g, rng, pools[static_cast<size_t>(cards[i])],
                     pools[static_cast<size_t>(cards[j])], 0.01f, true);
    }
  }

  // Output subset conceptually of South: connect Output <-> Dao and light South overlap.
  wire_bipartite(g, rng, pools[static_cast<size_t>(Region::Output)],
                 pools[static_cast<size_t>(Region::DaoYang)], 0.08f, false);
  wire_bipartite(g, rng, pools[static_cast<size_t>(Region::Output)],
                 pools[static_cast<size_t>(Region::DaoYin)], 0.08f, false);
  wire_bipartite(g, rng, pools[static_cast<size_t>(Region::Output)],
                 pools[static_cast<size_t>(Region::South)], 0.03f, false);

  // Safety: strip any accidental sediment non-dao edges / left-right edges.
  for (uint32_t i = 0; i < g.n; ++i) {
    auto& edges = g.out_edges[i];
    edges.erase(std::remove_if(edges.begin(), edges.end(),
                               [&](const Edge& e) {
                                 const Region rs = g.region[e.src];
                                 const Region rd = g.region[e.dst];
                                 if (is_sediment_region(rs) && is_sediment_region(rd)) return true;
                                 if (is_sediment_region(rs) && !is_dao_region(rd)) return true;
                                 if (is_sediment_region(rd) && !is_dao_region(rs)) return true;
                                 return false;
                               }),
                edges.end());
  }

  // Ensure every sediment has BOTH outbound to dao and inbound from dao.
  // Cross-hemisphere reachability (L→dao→R) requires dao→sediment coverage.
  std::uniform_int_distribution<size_t> dy(
      0, pools[static_cast<size_t>(Region::DaoYang)].size() - 1);
  std::uniform_int_distribution<size_t> dn(
      0, pools[static_cast<size_t>(Region::DaoYin)].size() - 1);

  auto has_out_to_dao = [&](uint32_t i) {
    for (const auto& e : g.out_edges[i]) {
      if (is_dao_region(g.region[e.dst])) return true;
    }
    return false;
  };
  auto has_in_from_dao = [&](uint32_t i) {
    for (uint32_t d : pools[static_cast<size_t>(Region::DaoYang)]) {
      if (edge_exists(g, d, i)) return true;
    }
    for (uint32_t d : pools[static_cast<size_t>(Region::DaoYin)]) {
      if (edge_exists(g, d, i)) return true;
    }
    return false;
  };

  for (Region sed : {Region::LeftSediment, Region::RightSediment}) {
    for (uint32_t i : pools[static_cast<size_t>(sed)]) {
      uint32_t d_yang = pools[static_cast<size_t>(Region::DaoYang)][dy(rng)];
      uint32_t d_yin = pools[static_cast<size_t>(Region::DaoYin)][dn(rng)];
      if (!has_out_to_dao(i)) {
        add_edge(g, i, d_yang, 0.35f, 2, 1);
        add_edge(g, i, d_yin, 0.35f, 2, 1);
      }
      if (!has_in_from_dao(i)) {
        add_edge(g, d_yang, i, 0.35f, 2, 1);
        add_edge(g, d_yin, i, 0.35f, 2, 1);
      }
    }
  }

  return g;
}

bool has_left_right_direct_edge(const Graph& g) {
  for (uint32_t i = 0; i < g.n; ++i) {
    if (!is_sediment_region(g.region[i])) continue;
    for (const auto& e : g.out_edges[i]) {
      if (is_sediment_region(g.region[e.dst]) && g.region[i] != g.region[e.dst]) {
        return true;
      }
    }
  }
  return false;
}

bool sediment_has_non_dao_neighbor(const Graph& g) {
  for (uint32_t i = 0; i < g.n; ++i) {
    if (!is_sediment_region(g.region[i])) continue;
    for (const auto& e : g.out_edges[i]) {
      if (!is_dao_region(g.region[e.dst])) return true;
    }
  }
  // Also check inbound from non-dao (stored only as out_edges; scan all).
  for (uint32_t i = 0; i < g.n; ++i) {
    if (is_dao_region(g.region[i]) || is_sediment_region(g.region[i])) continue;
    for (const auto& e : g.out_edges[i]) {
      if (is_sediment_region(g.region[e.dst])) return true;
    }
  }
  return false;
}

bool all_cross_paths_via_dao(const Graph& g) {
  // No left-right direct edges and sediments only touch dao => any L->R path must enter dao.
  if (has_left_right_direct_edge(g)) return false;
  if (sediment_has_non_dao_neighbor(g)) return false;

  std::vector<uint32_t> left, right, dao;
  for (uint32_t i = 0; i < g.n; ++i) {
    if (g.region[i] == Region::LeftSediment) left.push_back(i);
    if (g.region[i] == Region::RightSediment) right.push_back(i);
    if (is_dao_region(g.region[i])) dao.push_back(i);
  }
  if (left.empty() || right.empty() || dao.empty()) return false;

  std::unordered_set<uint32_t> dao_set(dao.begin(), dao.end());

  // Sample a few pairs: path exists with dao; path without dao nodes must fail.
  const size_t samples = std::min<size_t>(8, std::min(left.size(), right.size()));
  for (size_t s = 0; s < samples; ++s) {
    uint32_t L = left[s];
    uint32_t R = right[s];
    if (!can_reach(g, L, R)) return false;
    if (!can_reach(g, R, L)) return false;
    if (can_reach_avoiding(g, L, R, dao_set)) return false;
    if (can_reach_avoiding(g, R, L, dao_set)) return false;
  }
  return true;
}

uint32_t count_isolated(const Graph& g) {
  std::vector<uint32_t> indeg(g.n, 0);
  for (uint32_t i = 0; i < g.n; ++i) {
    for (const auto& e : g.out_edges[i]) {
      if (e.dst < g.n) indeg[e.dst]++;
    }
  }
  uint32_t iso = 0;
  for (uint32_t i = 0; i < g.n; ++i) {
    if (g.out_edges[i].empty() && indeg[i] == 0) ++iso;
  }
  return iso;
}

}  // namespace dao
