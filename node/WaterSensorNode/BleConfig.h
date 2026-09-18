#pragma once
#include <Arduino.h>
#include "NodeConfig.h"

class NodeBleConfig {
public:
  bool begin(NodeConfig &cfg);
  void loop(NodeConfig &cfg);
  bool active() const { return active_; }
  bool timedOut() const;
  void stop();

private:
  bool active_ = false;
  uint32_t startedMs_ = 0;
  String pending_;
  NodeConfig *cfg_ = nullptr;

  String toJson(const NodeConfig &cfg);
  bool applyJson(NodeConfig &cfg, const String &json);
  void handleCommand(const String &cmd);

  static NodeBleConfig *instance_;
  friend class NodeBleConfigCallbacks;
};
