#pragma once

#include "event_queue.h"
#include "graph.h"

namespace dao {

struct ConductionReport {
  bool ok = false;
  uint32_t isolated = 0;
  bool left_right_direct = false;         // expect false
  bool sediment_non_dao_neighbor = false; // expect false
  bool all_cross_via_dao = false;         // expect true
  bool long_delay_ok = false;             // expect true
};

ConductionReport validate_conduction(const Graph& g, EventQueue& scratch);

void print_conduction_report(const ConductionReport& r);

}  // namespace dao
