#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "HubConfig.h"
#include "HubStorage.h"

class Cloud {
public:
  void begin(const HubConfig &cfg);
  void sendTelemetry(const Telemetry &t, const HubConfig &cfg);
  void refreshConfig(HubConfig &cfg, HubStorage &storage);
private:
  void connectWiFi();
};
