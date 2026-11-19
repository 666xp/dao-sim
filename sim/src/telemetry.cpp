#include "telemetry.h"

#include <cstring>

namespace dao {

void TelemetryHub::publish(const TelemetrySample& s) {
  std::lock_guard<std::mutex> lock(mu_);
  frame_.tick = s.tick;
  frame_.D = s.D;
  frame_.sigma = s.sigma;
  for (int i = 0; i < 6; ++i) frame_.rates[i] = s.rates[i];
  frame_.out_bits = s.out_bits;
  has_ = true;
}

bool TelemetryHub::consume(TelemetryFrame& out) {
  std::lock_guard<std::mutex> lock(mu_);
  if (!has_) return false;
  out = frame_;
  has_ = false;
  return true;
}

std::vector<uint8_t> pack_frame(const TelemetryFrame& f) {
  std::vector<uint8_t> bytes(sizeof(TelemetryFrame));
  std::memcpy(bytes.data(), &f, sizeof(TelemetryFrame));
  return bytes;
}

}  // namespace dao
