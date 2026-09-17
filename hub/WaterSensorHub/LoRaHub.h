#pragma once
#include <Arduino.h>
#include <RadioLib.h>
#include "HubConfig.h"

class LoRaHub {
public:
  LoRaHub();
  bool begin(const HubConfig &cfg);
  bool pollNode(const NodeConfig &node, Telemetry &out, const HubConfig &cfg);

private:
  Module *mod = nullptr;
  SX1262 *radio = nullptr;
  void configureRadio(const HubConfig &cfg);
  bool transmitPoll(const NodeConfig &node, const HubConfig &cfg);
  bool receiveTelemetry(Telemetry &out, uint32_t timeoutMs);
  bool parseTelemetry(const String &s, Telemetry &out);
};
