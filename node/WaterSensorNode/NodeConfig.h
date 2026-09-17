#pragma once
#include <Arduino.h>

struct NodeConfig {
  uint8_t networkId;
  uint8_t hubId;
  uint8_t nodeId;
  uint32_t reportIntervalSec;
  uint32_t frequencyHz;
  uint8_t spreadingFactor;
  uint32_t bandwidthHz;
  uint8_t codingRate;
  int8_t txPowerDbm;
  uint16_t configVersion;
};

bool loadConfig(NodeConfig& cfg);
bool saveConfig(const NodeConfig& cfg);
