#pragma once
#include <Arduino.h>

static constexpr uint8_t MAX_NODES = 10;

struct NodeConfig {
  bool enabled = false;
  uint8_t nodeId = 0;
  uint32_t pollTimeoutMs = 2500;
};

struct HubConfig {
  String hubId = "HUB01";
  String networkId = "WATER01";

  String wifiSsid = "";
  String wifiPassword = "";
  String googleScriptUrl = "";

  float frequencyMHz = 915.0f;
  float bandwidthKHz = 125.0f;
  uint8_t spreadingFactor = 7;
  uint8_t codingRate = 5;
  int8_t txPowerDbm = 14;
  uint16_t preambleSymbols = 1465;
  uint32_t pollIntervalSec = 300;
  uint16_t configVersion = 1;

  uint8_t nodeCount = 5;
  NodeConfig nodes[MAX_NODES];
};

struct Telemetry {
  uint8_t nodeId = 0;
  uint32_t sequence = 0;
  int32_t tempMv = 0;
  int32_t doMv = 0;
  uint8_t waterFlow = 0;
  uint8_t airFlow = 0;
  int32_t batteryMv = 0;
  int16_t rssi = 0;
  int8_t snr = 0;
  uint16_t configVersion = 0;
};
