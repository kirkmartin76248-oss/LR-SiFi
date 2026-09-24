#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <Preferences.h>
#include <time.h>
#include "../../protocol/espnow_protocol.h"

using namespace KrekProtocol;

// XIAO ESP32-C6 V1.0 — locked RF switch mapping.
// GPIO3: LOW = RF switch ON, HIGH = OFF.
// GPIO14: LOW = RF1, HIGH = RF2.
// RF2 is the U.FL/external antenna path.
static constexpr int PIN_RF_SWITCH_POWER = 3;
static constexpr int PIN_RF_SWITCH_SELECT = 14;

static constexpr uint32_t BACKEND_RETRY_MS = 5000;
static constexpr uint32_t NTP_SYNC_TIMEOUT_MS = 15000;
static constexpr size_t MAX_PENDING_UPDATES = 16;
static constexpr char NTP_SERVER[] = "pool.ntp.org";
static constexpr size_t CONFIG_FINGERPRINT_LEN = KrekProtocol::CONFIG_FINGERPRINT_LEN;

enum class HubState : uint8_t {
  BOOT,
  LOAD_CONFIG,
  RF_SWITCH,
  BLE_WINDOW,
  WIFI,
  ESPNOW,
  RUN,
  FAULT
};

struct PendingUpdate {
  bool valid;
  char node_id[NODE_ID_LEN];
  uint32_t revision;
  uint8_t config_fingerprint[CONFIG_FINGERPRINT_LEN];
  ConfigUpdate packet;
};

struct HubRuntime {
  HubState state = HubState::BOOT;
  Preferences prefs;
  PendingUpdate pending[MAX_PENDING_UPDATES]{};
  uint32_t last_backend_attempt_ms = 0;
  bool ntp_time_valid = false;
};

static HubRuntime rt;

static void setupRfSwitch() {
  // Keep RF switch disabled while GPIO directions/states are established.
  pinMode(PIN_RF_SWITCH_POWER, OUTPUT);
  digitalWrite(PIN_RF_SWITCH_POWER, HIGH);

  pinMode(PIN_RF_SWITCH_SELECT, OUTPUT);
  digitalWrite(PIN_RF_SWITCH_SELECT, HIGH); // RF2 / U.FL

  digitalWrite(PIN_RF_SWITCH_POWER, LOW);   // enable switch
}

static int findPending(const char* nodeId) {
  for (size_t i = 0; i < MAX_PENDING_UPDATES; ++i) {
    if (rt.pending[i].valid && strncmp(rt.pending[i].node_id, nodeId, NODE_ID_LEN) == 0) {
      return (int)i;
    }
  }
  return -1;
}

static int allocatePending(const char* nodeId) {
  int existing = findPending(nodeId);
  if (existing >= 0) return existing;

  for (size_t i = 0; i < MAX_PENDING_UPDATES; ++i) {
    if (!rt.pending[i].valid) {
      rt.pending[i].valid = true;
      memset(rt.pending[i].node_id, 0, NODE_ID_LEN);
      strncpy(rt.pending[i].node_id, nodeId, NODE_ID_LEN - 1);
      return (int)i;
    }
  }
  return -1;
}

static void removePending(int index) {
  if (index < 0 || index >= (int)MAX_PENDING_UPDATES) return;
  memset(&rt.pending[index], 0, sizeof(PendingUpdate));
}

static uint64_t hubUtcMillis() {
  time_t now = time(nullptr);
  if (now < 1700000000) return 0; // Not synchronized yet.
  return (uint64_t)now * 1000ULL;
}

static bool synchronizeNtp() {
  configTime(0, 0, NTP_SERVER);
  const uint32_t start = millis();
  while (millis() - start < NTP_SYNC_TIMEOUT_MS) {
    if (time(nullptr) >= 1700000000) {
      rt.ntp_time_valid = true;
      return true;
    }
    delay(100);
  }
  rt.ntp_time_valid = false;
  return false;
}

// This callback is intentionally limited to the fast ESP-NOW path.
// No Wi-Fi, HTTPS, Preferences, or slow work belongs here.
static void onEspNowReceive(const esp_now_recv_info_t* info, const uint8_t* data, int len) {
  if (!info || !data || len < (int)sizeof(Header)) return;

  const Header* h = reinterpret_cast<const Header*>(data);
  if (h->magic != MAGIC || h->version != VERSION) return;

  if (h->type == TELEMETRY && len == (int)sizeof(Telemetry)) {
    const Telemetry* t = reinterpret_cast<const Telemetry*>(data);

    // Immediate ACK: same node identity and sequence.
    TelemetryAck ack{};
    ack.h.magic = MAGIC;
    ack.h.version = VERSION;
    ack.h.type = TELEMETRY_ACK;
    ack.h.message_id = h->message_id;
    ack.h.sequence = h->sequence;
    memcpy(ack.h.node_id, h->node_id, NODE_ID_LEN);
    ack.acknowledged_sequence = t->h.sequence;

    // If a pending update exists and its fingerprint differs from the Node's
    // reported fingerprint, tell the Node to request the complete config.
    const int pendingIdx = findPending(t->h.node_id);
    ack.config_changed = 0;
    memcpy(ack.config_fingerprint, t->config_fingerprint, CONFIG_FINGERPRINT_LEN);
    if (pendingIdx >= 0 && memcmp(rt.pending[pendingIdx].config_fingerprint,
                                  t->config_fingerprint,
                                  CONFIG_FINGERPRINT_LEN) != 0) {
      ack.config_changed = 1;
      memcpy(ack.config_fingerprint,
             rt.pending[pendingIdx].config_fingerprint,
             CONFIG_FINGERPRINT_LEN);
    }

    esp_now_send(info->src_addr, reinterpret_cast<const uint8_t*>(&ack), sizeof(ack));

    // Backend forwarding is queued for the main loop.
    // Deduplication and RSSI capture belong in the queued record.
    // info->rx_ctrl->rssi is the Hub-measured RSSI source.
  }

  if (h->type == UPDATE_QUERY && len == (int)sizeof(UpdateQuery)) {
    const UpdateQuery* q = reinterpret_cast<const UpdateQuery*>(data);
    const int idx = findPending(q->h.node_id);

    if (idx < 0) {
      NoUpdate reply{};
      reply.h.magic = MAGIC;
      reply.h.version = VERSION;
      reply.h.type = NO_UPDATE;
      reply.h.message_id = q->h.message_id;
      reply.h.sequence = q->h.sequence;
      memcpy(reply.h.node_id, q->h.node_id, NODE_ID_LEN);
      reply.current_config_revision = q->current_config_revision;
      memcpy(reply.config_fingerprint, q->config_fingerprint, CONFIG_FINGERPRINT_LEN);
      esp_now_send(info->src_addr, reinterpret_cast<const uint8_t*>(&reply), sizeof(reply));
    } else {
      ConfigUpdate update = rt.pending[idx].packet;
      esp_now_send(info->src_addr, reinterpret_cast<const uint8_t*>(&update), sizeof(update));
    }
  }

  if (h->type == CONFIG_ACK && len == (int)sizeof(ConfigAck)) {
    const ConfigAck* ack = reinterpret_cast<const ConfigAck*>(data);
    const int idx = findPending(ack->h.node_id);
    if (idx >= 0 && rt.pending[idx].revision == ack->acknowledged_config_revision) {
      removePending(idx);
    }
  }
}

static bool beginEspNow() {
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    return false;
  }

  esp_now_register_recv_cb(onEspNowReceive);
  return true;
}

static void enterProvisioningWindow() {
  // BLE provisioning implementation is deliberately isolated from
  // the ESP-NOW receive path. The Installer reads the Hub MAC here.
}

static void loadPersistentConfiguration() {
  // Hub identity, Wi-Fi credentials, backend URL, pending mailbox,
  // and persistent offline telemetry queue are loaded here.
  // The Hub still does not load a Node list.
}

static void serviceBackendQueue() {
  // Asynchronous HTTPS/Apps Script processing belongs here.
  // A slow backend must never block ESP-NOW ACKs.
  // Each queued record carries receivedAtUtcMs and Hub RSSI.
  // Records are retained locally until backend acceptance.
}

static void serviceWiFi() {
  // Maintain customer Wi-Fi connection independently of ESP-NOW.
}

void setup() {
  Serial.begin(115200);
  delay(100);

  rt.state = HubState::LOAD_CONFIG;
  loadPersistentConfiguration();

  rt.state = HubState::RF_SWITCH;
  setupRfSwitch();

  rt.state = HubState::BLE_WINDOW;
  enterProvisioningWindow();

  rt.state = HubState::WIFI;
  serviceWiFi();
  synchronizeNtp();

  rt.state = HubState::ESPNOW;
  if (!beginEspNow()) {
    rt.state = HubState::FAULT;
    return;
  }

  rt.state = HubState::RUN;
}

void loop() {
  if (rt.state == HubState::FAULT) {
    delay(1000);
    return;
  }

  serviceWiFi();
  serviceBackendQueue();
  delay(1);
}
