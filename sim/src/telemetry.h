#pragma once

#include "sim_world.h"

#include <cstdint>
#include <mutex>
#include <vector>

namespace dao {

#pragma pack(push, 1)
struct TelemetryFrame {
  uint64_t tick;
  float D;
  float sigma;
  float rates[6];
  uint16_t out_bits;
};
#pragma pack(pop)

static_assert(sizeof(TelemetryFrame) == 8 + 4 + 4 + 24 + 2, "telemetry frame layout");

class TelemetryHub {
 public:
  void publish(const TelemetrySample& s);
  bool consume(TelemetryFrame& out);

 private:
  std::mutex mu_;
  bool has_ = false;
  TelemetryFrame frame_{};
};

std::vector<uint8_t> pack_frame(const TelemetryFrame& f);

}  // namespace dao
