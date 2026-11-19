#pragma once

#include "sim_world.h"
#include "telemetry.h"

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <thread>

namespace dao {

class WsServer {
 public:
  WsServer(SimWorld& world, TelemetryHub& hub, int port = 3848);
  ~WsServer();

  void start();
  void stop();

 private:
  void net_thread_main();
  void handle_json(const std::string& msg);

  SimWorld& world_;
  TelemetryHub& hub_;
  int port_;
  std::atomic<bool> stop_{false};
  std::thread thr_;
  void* server_ = nullptr;  // ix::WebSocketServer*
};

}  // namespace dao
