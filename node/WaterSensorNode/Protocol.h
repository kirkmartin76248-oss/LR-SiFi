#pragma once
#include <Arduino.h>
constexpr uint8_t PROTOCOL_VERSION = 1;

enum PacketType : uint8_t { PKT_POLL=1, PKT_TELEMETRY=2, PKT_ACK=3 };
#pragma pack(push,1)
struct PacketHeader { uint8_t version; uint8_t type; uint8_t networkId; uint8_t hubId; uint8_t nodeId; uint32_t sequence; };
struct PollPacket { PacketHeader h; };
struct TelemetryPacket {
  uint8_t version; uint8_t type; uint8_t networkId; uint8_t hubId; uint8_t nodeId; uint32_t sequence;
  uint16_t temperatureMv; uint16_t dissolvedOxygenMv; uint8_t waterFlow; uint8_t airFlow; uint16_t batteryMv; uint16_t configVersion;
};
struct AckPacket {
  uint8_t version; uint8_t type; uint8_t networkId; uint8_t hubId; uint8_t nodeId; uint32_t sequence;
  uint16_t configVersion; uint32_t reportIntervalSec; int8_t txPowerDbm;
};
#pragma pack(pop)
