#include <catch2/catch_test_macros.hpp>

#include "event_queue.h"

TEST_CASE("long delay 1e6 tick delivery", "[queue]") {
  dao::EventQueue q;
  dao::SpikeEvent e;
  e.delivery_tick = 1000000ull;
  e.target = 3;
  e.amp = 2.5f;
  q.push(e);

  std::vector<dao::SpikeEvent> early;
  REQUIRE(q.advance_to(999999ull, early));
  REQUIRE(early.empty());
  REQUIRE(q.size() == 1);

  std::vector<dao::SpikeEvent> late;
  REQUIRE(q.advance_to(1000000ull, late));
  REQUIRE(late.size() == 1);
  REQUIRE(late[0].delivery_tick == 1000000ull);
  REQUIRE(late[0].target == 3);
  REQUIRE(late[0].amp == 2.5f);
  REQUIRE(q.empty());
}
