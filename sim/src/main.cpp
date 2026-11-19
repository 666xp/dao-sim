#include "sim_world.h"
#include "telemetry.h"
#include "ws_server.h"

#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <string>
#include <thread>

namespace {
std::atomic<bool> g_stop{false};
void on_sig(int) { g_stop = true; }
}  // namespace

int main(int argc, char** argv) {
  bool fake = false;
  int port = 3848;
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    if (a == "--fake") fake = true;
    if (a == "--port" && i + 1 < argc) port = std::stoi(argv[++i]);
  }

  std::signal(SIGINT, on_sig);
  std::signal(SIGTERM, on_sig);

  dao::SimConfig cfg;
  cfg.seed = 42;
  cfg.dt_ms = 1.0f;
  cfg.fake_telemetry = fake;

  dao::SimWorld world(cfg);
  dao::TelemetryHub hub;
  dao::WsServer server(world, hub, port);
  server.start();

  world.start();
  std::cerr << "sim_engine start fake=" << (fake ? 1 : 0) << " port=" << port << "\n";

  uint64_t last_pub = 0;
  while (!g_stop.load()) {
    if (world.running()) {
      world.step(1);
      if (world.tick() - last_pub >= 10) {
        hub.publish(world.sample());
        last_pub = world.tick();
      }
    } else {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  }

  server.stop();
  return 0;
}
