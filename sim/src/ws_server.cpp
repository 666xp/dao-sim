#include "ws_server.h"

#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXWebSocket.h>
#include <ixwebsocket/IXWebSocketServer.h>

#include <chrono>
#include <iostream>
#include <sstream>

namespace dao {
namespace {

std::string json_get_string(const std::string& msg, const std::string& key) {
  const std::string pat = "\"" + key + "\"";
  auto p = msg.find(pat);
  if (p == std::string::npos) return {};
  p = msg.find(':', p);
  if (p == std::string::npos) return {};
  p = msg.find('"', p);
  if (p == std::string::npos) return {};
  auto q = msg.find('"', p + 1);
  if (q == std::string::npos) return {};
  return msg.substr(p + 1, q - p - 1);
}

bool json_get_bool(const std::string& msg, const std::string& key, bool* ok) {
  const std::string pat = "\"" + key + "\"";
  auto p = msg.find(pat);
  if (p == std::string::npos) {
    if (ok) *ok = false;
    return false;
  }
  p = msg.find(':', p);
  if (p == std::string::npos) {
    if (ok) *ok = false;
    return false;
  }
  if (msg.find("true", p) < msg.find("false", p)) {
    if (ok) *ok = true;
    return true;
  }
  if (ok) *ok = true;
  return false;
}

double json_get_number(const std::string& msg, const std::string& key, bool* ok) {
  const std::string pat = "\"" + key + "\"";
  auto p = msg.find(pat);
  if (p == std::string::npos) {
    if (ok) *ok = false;
    return 0;
  }
  p = msg.find(':', p);
  if (p == std::string::npos) {
    if (ok) *ok = false;
    return 0;
  }
  ++p;
  while (p < msg.size() && (msg[p] == ' ' || msg[p] == '\t')) ++p;
  try {
    size_t n = 0;
    double v = std::stod(msg.substr(p), &n);
    if (ok) *ok = n > 0;
    return v;
  } catch (...) {
    if (ok) *ok = false;
    return 0;
  }
}

Region parse_group(const std::string& s) {
  if (s == "east") return Region::East;
  if (s == "west") return Region::West;
  if (s == "south") return Region::South;
  if (s == "north") return Region::North;
  if (s == "up") return Region::Up;
  if (s == "down") return Region::Down;
  return Region::East;
}

}  // namespace

WsServer::WsServer(SimWorld& world, TelemetryHub& hub, int port)
    : world_(world), hub_(hub), port_(port) {}

WsServer::~WsServer() { stop(); }

void WsServer::handle_json(const std::string& msg) {
  const std::string cmd = json_get_string(msg, "cmd");
  if (cmd == "start") {
    world_.start();
  } else if (cmd == "pause") {
    world_.pause();
  } else if (cmd == "step") {
    // Only safe while paused: sim thread must not be advancing world.
    if (world_.running()) {
      std::cerr << "ws step ignored: engine is running (pause first)\n";
      return;
    }
    bool ok = false;
    uint64_t n = static_cast<uint64_t>(json_get_number(msg, "n", &ok));
    if (!ok || n == 0) n = 1;
    if (n > 10000) n = 10000;
    world_.step(n);
    world_.request_publish();  // sim thread pushes one frame; hub stays single-producer
  } else if (cmd == "seed") {
    bool ok = false;
    const uint64_t v = static_cast<uint64_t>(json_get_number(msg, "value", &ok));
    if (ok) world_.set_seed(v);
  } else if (cmd == "ablation") {
    const std::string name = json_get_string(msg, "name");
    bool ok = false;
    const bool on = json_get_bool(msg, "on", &ok);
    AblationFlags f = world_.ablation();
    if (name == "dao_adaptation" && ok) f.dao_adaptation = on;
    if (name == "noise" && ok) f.noise = on;
    world_.set_ablation(f);
  } else if (cmd == "perturb") {
    const std::string group = json_get_string(msg, "group");
    bool ok = false;
    const uint32_t pulses = static_cast<uint32_t>(json_get_number(msg, "pulses", &ok));
    world_.perturb(parse_group(group), ok ? pulses : 50);
  } else if (cmd == "param") {
    const std::string name = json_get_string(msg, "name");
    bool ok = false;
    const double v = json_get_number(msg, "value", &ok);
    if (ok && name == "g") world_.set_g(static_cast<float>(v));
  }
}

void WsServer::net_thread_main() {
  ix::initNetSystem();
  auto server = std::make_unique<ix::WebSocketServer>(port_, "0.0.0.0");
  server_ = server.get();

  server->setOnConnectionCallback(
      [this](std::weak_ptr<ix::WebSocket> wp, std::shared_ptr<ix::ConnectionState>) {
        auto ws = wp.lock();
        if (!ws) return;
        ws->setOnMessageCallback([this, wp](const ix::WebSocketMessagePtr& msg) {
          if (msg->type == ix::WebSocketMessageType::Message) {
            handle_json(msg->str);
          }
        });
      });

  auto res = server->listen();
  if (!res.first) {
    std::cerr << "ws listen failed: " << res.second << "\n";
    return;
  }
  server->start();
  std::cerr << "ws listening on " << port_ << "\n";

  while (!stop_.load()) {
    TelemetryFrame frame{};
    if (hub_.consume(frame)) {
      auto bytes = pack_frame(frame);
      // Broadcast binary to all clients.
      auto clients = server->getClients();
      for (auto& c : clients) {
        c->sendBinary(std::string(reinterpret_cast<const char*>(bytes.data()), bytes.size()));
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }

  server->stop();
  server_ = nullptr;
  ix::uninitNetSystem();
}

void WsServer::start() {
  stop_ = false;
  thr_ = std::thread([this] { net_thread_main(); });
}

void WsServer::stop() {
  stop_ = true;
  if (thr_.joinable()) thr_.join();
}

}  // namespace dao
