#include "event_queue.h"
#include "graph.h"
#include "validate.h"

#include <cstdlib>
#include <iostream>

int main(int argc, char** argv) {
  uint64_t seed = 42;
  if (argc > 1) {
    seed = static_cast<uint64_t>(std::strtoull(argv[1], nullptr, 10));
  }

  dao::Graph g = dao::build_default_graph(seed);
  dao::EventQueue q;
  dao::ConductionReport r = dao::validate_conduction(g, q);
  dao::print_conduction_report(r);

  std::cerr << "neurons=" << g.n << " seed=" << seed << "\n";
  return r.ok ? 0 : 1;
}
