#pragma once
#include <stdint.h>
#include <stddef.h>

namespace KrekProtocol {

static constexpr uint16_t MAGIC = 0x4B4C;
static constexpr uint8_t VERSION = 1;
static constexpr size_t NODE_ID_LEN = 16;
static constexpr size_t HUB_ID_LEN = 16;
static constexpr size_t PORT_COUNT = 4;
static constexpr size_t CONFIG_FINGERPRINT_LEN = 4;

enum MessageType : uint8_t {
  TELEMETRY = 1,
  TELEMETRY_ACK = 2,
  UPDATE_QUERY = 3,
  NO_UPDATE = 4,
  CONFIG_UPDATE = 5,
  CONFIG_ACK = 6
};

enum SignalMode : uint8_t {
  MODE_ANALOG = 0,
  MODE_DIGITAL = 1
};

enum ActiveLevel : uint8_t {
  ACTIVE_HIGH = 0,
  ACTIVE_LOW = 1
};

enum SensorType : uint8_t {
  SENSOR_DISABLED = 0,
  SENSOR_WATER_TEMPERATURE = 1,
  SENSOR_DISSOLVED_OXYGEN = 2,
  SENSOR_WATER_FLOW = 3,
  SENSOR_AIR_FLOW = 4,
  SENSOR_OTHER = 255
};

#pragma pack(push, 1)

struct Header {
  uint16_t magic;
  uint8_t version;
  uint8_t type;
  uint32_t message_id;
  uint32_t sequence;
  char node_id[NODE_ID_LEN];
};

struct Telemetry {
  Header h;
  uint32_t config_revision;
  uint8_t config_fingerprint[CONFIG_FINGERPRINT_LEN];
  uint32_t node_uptime_s;
  uint16_t battery_mv;
  uint8_t present_mask;
  uint8_t reserved;
  int32_t values_mv_or_state[PORT_COUNT];
};

struct TelemetryAck {
  Header h;
  uint32_t acknowledged_sequence;
  uint8_t config_fingerprint[CONFIG_FINGERPRINT_LEN];
  uint8_t config_changed;
};

struct UpdateQuery {
  Header h;
  uint32_t current_config_revision;
  uint8_t config_fingerprint[CONFIG_FINGERPRINT_LEN];
};

struct NoUpdate {
  Header h;
  uint32_t current_config_revision;
  uint8_t config_fingerprint[CONFIG_FINGERPRINT_LEN];
};

struct PortConfig {
  uint8_t enabled;
  uint8_t sensor_type;
  uint8_t signal_mode;
  uint8_t active_level;
};

struct ConfigUpdate {
  Header h;
  uint32_t config_revision;
  uint8_t config_fingerprint[CONFIG_FINGERPRINT_LEN];
  char hub_id[HUB_ID_LEN];
  uint8_t hub_mac[6];
  uint32_t reporting_interval_s;
  uint32_t sensor_delay_ms;
  PortConfig ports[PORT_COUNT];
};

struct ConfigAck {
  Header h;
  uint32_t acknowledged_config_revision;
  uint8_t config_fingerprint[CONFIG_FINGERPRINT_LEN];
};

#pragma pack(pop)

static_assert(sizeof(Telemetry) < 250, "Telemetry exceeds ESP-NOW payload limit");
static_assert(sizeof(ConfigUpdate) < 250, "Config update exceeds ESP-NOW payload limit");

} // namespace KrekProtocol
