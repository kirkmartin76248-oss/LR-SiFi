#pragma once
#include <Arduino.h>
#include <SPI.h>
#include <RadioLib.h>
#include "NodeConfig.h"
#include "Protocol.h"

class LoRaRadio {
public:
  LoRaRadio();
  bool begin(const NodeConfig& cfg);
  void applyConfig(const NodeConfig& cfg);
  bool receivePoll(PacketHeader& hdr, PollPacket& poll);
  bool sendTelemetry(const TelemetryPacket& packet);
  bool receiveAck(AckPacket& packet, uint32_t timeoutMs);
  bool startDutyCycleRX();
private:
  SX1262 radio;
  NodeConfig activeCfg;
  static void dio1ISR();
  bool readPacket(uint8_t* buf, size_t len, uint32_t timeoutMs);
};
