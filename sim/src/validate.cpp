#include "validate.h"

#include <iostream>

namespace dao {

ConductionReport validate_conduction(const Graph& g, EventQueue& scratch) {
  ConductionReport r;
  r.isolated = count_isolated(g);
  r.left_right_direct = has_left_right_direct_edge(g);
  r.sediment_non_dao_neighbor = sediment_has_non_dao_neighbor(g);
  r.all_cross_via_dao = all_cross_paths_via_dao(g);

  // Long-delay delivery: inject event at 1e6, advance_to, confirm delivery.
  while (!scratch.empty()) scratch.pop();
  SpikeEvent e;
  e.delivery_tick = 1000000ull;
  e.target = g.n > 0 ? 0u : 0u;
  e.amp = 1.0f;
  scratch.push(e);
  // Should not deliver early.
  std::vector<SpikeEvent> early;
  scratch.advance_to(999999ull, early);
  std::vector<SpikeEvent> late;
  scratch.advance_to(1000000ull, late);
  r.long_delay_ok = early.empty() && late.size() == 1 && late[0].delivery_tick == 1000000ull &&
                    late[0].amp == 1.0f;

  r.ok = (r.isolated == 0) && !r.left_right_direct && !r.sediment_non_dao_neighbor &&
         r.all_cross_via_dao && r.long_delay_ok;
  return r;
}

void print_conduction_report(const ConductionReport& r) {
  auto line = [](bool pass, const char* name, auto value) {
    std::cout << (pass ? "PASS" : "FAIL") << ": " << name << " " << value << "\n";
  };
  line(r.isolated == 0, "isolated_count", r.isolated);
  line(!r.left_right_direct, "left_right_direct", r.left_right_direct ? 1 : 0);
  line(!r.sediment_non_dao_neighbor, "sediment_non_dao_neighbor",
       r.sediment_non_dao_neighbor ? 1 : 0);
  line(r.all_cross_via_dao, "all_cross_via_dao", r.all_cross_via_dao ? 1 : 0);
  line(r.long_delay_ok, "long_delay_delivery", r.long_delay_ok ? 1000000 : 0);
  line(r.ok, "conduction_ok", r.ok ? 1 : 0);
}

}  // namespace dao
