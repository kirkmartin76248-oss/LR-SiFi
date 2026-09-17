#include "LoRaHub.h"

static constexpr int PIN_NSS   = 21;
static constexpr int PIN_DIO1  = 7;
static constexpr int PIN_RESET = 16;
static constexpr int PIN_BUSY  = 17;

LoRaHub::LoRaHub() {
  mod = new Module(PIN_NSS, PIN_DIO1, PIN_RESET, PIN_BUSY);
  radio = new SX1262(mod);
}

bool LoRaHub::begin(const HubConfig &cfg) {
  int state = radio->begin(cfg.frequencyMHz,
                           cfg.bandwidthKHz,
                           cfg.spreadingFactor,
                           cfg.codingRate,
                           0x12,
                           cfg.txPowerDbm,
                           cfg.preambleSymbols);
  if (state != RADIOLIB_ERR_NONE) return false;
  radio->setCRC(true);
  return true;
}

bool LoRaHub::pollNode(const NodeConfig &node, Telemetry &out, const HubConfig &cfg) {
  if (!transmitPoll(node, cfg)) return false;
  return receiveTelemetry(out, node.pollTimeoutMs);
}

bool LoRaHub::transmitPoll(const NodeConfig &node, const HubConfig &cfg) {
  String msg = "P," + String(cfg.networkId) + ",N" + String(node.nodeId);
  int state = radio->transmit(msg);
  return state == RADIOLIB_ERR_NONE;
}

bool LoRaHub::receiveTelemetry(Telemetry &out, uint32_t timeoutMs) {
  String msg;
  int state = radio->receive(msg, timeoutMs);
  if (state != RADIOLIB_ERR_NONE) return false;
  out.rssi = radio->getRSSI();
  out.snr  = radio->getSNR();
  return parseTelemetry(msg, out);
}

bool LoRaHub::parseTelemetry(const String &s, Telemetry &out) {
  int p = 0;
  String tok[9];
  uint8_t n = 0;
  while (p < s.length() && n < 9) {
    int q = s.indexOf(',', p);
    if (q < 0) q = s.length();
    tok[n++] = s.substring(p, q);
    p = q + 1;
  }
  if (n < 8 || tok[0] != "R") return false;
  for (uint8_t i = 1; i < n; ++i) {
    if (tok[i].startsWith("N")) out.nodeId = tok[i].substring(1).toInt();
    else if (tok[i].startsWith("SEQ")) out.sequence = tok[i].substring(3).toInt();
    else if (tok[i].startsWith("TEMP")) out.tempMv = tok[i].substring(4).toInt();
    else if (tok[i].startsWith("DO")) out.doMv = tok[i].substring(2).toInt();
    else if (tok[i].startsWith("WF")) out.waterFlow = tok[i].substring(2).toInt();
    else if (tok[i].startsWith("AF")) out.airFlow = tok[i].substring(2).toInt();
    else if (tok[i].startsWith("BAT")) out.batteryMv = tok[i].substring(3).toInt();
    else if (tok[i].startsWith("CV")) out.configVersion = tok[i].substring(2).toInt();
  }
  return out.nodeId != 0;
}
