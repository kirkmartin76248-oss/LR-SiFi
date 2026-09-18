#pragma once
#include <Arduino.h>
#include "HubConfig.h"
#include "HubStorage.h"

class HubBleConfig {
public:
  bool begin(HubConfig &cfg, HubStorage &storage);
  void loop(HubConfig &cfg, HubStorage &storage);
  bool active() const { return active_; }
  bool timedOut() const;
  void stop();

private:
  bool active_ = false;
  uint32_t startedMs_ = 0;
  String pending_;
  HubConfig *cfg_ = nullptr;
  HubStorage *storage_ = nullptr;

  String toJson(const HubConfig &cfg);
  bool applyJson(HubConfig &cfg, const String &json);
  void handleCommand(const String &cmd);

  static HubBleConfig *instance_;
  friend class HubBleConfigCallbacks;
};
