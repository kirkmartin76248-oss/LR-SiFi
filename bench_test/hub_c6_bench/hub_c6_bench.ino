/*
  KREK LABS - WiFi William
  XIAO ESP32-C6 HUB BENCH RECEIVER

  Bench purpose:
    - Receive Node telemetry
    - Display decoded telemetry in Serial
    - Display RSSI
    - Send immediate ACK

  This bench Hub intentionally does NOT include:
    - Google / Wi-Fi upload
    - NTP
    - configuration mailbox
    - BLE configuration
    - customer monitoring

  Those belong to the full Hub firmware. This build keeps the
  current-consumption / ESP-NOW bench test simple.
*/

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

#define ESPNOW_CHANNEL 1

struct Telemetry {
  uint8_t type;
  uint32_t sequence;
  uint32_t fingerprint;
  uint32_t previousWakeDurationMs;
  uint16_t batteryMv;
  uint16_t port1Mv;
  uint16_t port2Mv;
  uint16_t port3Mv;
  uint16_t port4Mv;
};

void printMac(const uint8_t *mac) {
  for (int i = 0; i < 6; ++i) {
    if (i) Serial.print(":");
    if (mac[i] < 16) Serial.print("0");
    Serial.print(mac[i], HEX);
  }
}

void sendAck(const uint8_t *nodeMac, uint32_t sequence) {
  uint8_t ack[5];
  ack[0] = 0x02;
  memcpy(ack + 1, &sequence, sizeof(uint32_t));

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, nodeMac, 6);
  peer.channel = ESPNOW_CHANNEL;
  peer.encrypt = false;

  if (!esp_now_is_peer_exist(nodeMac)) {
    esp_err_t addResult = esp_now_add_peer(&peer);
    if (addResult != ESP_OK && addResult != ESP_ERR_ESPNOW_EXIST) {
      Serial.printf("[%lu ms] ACK peer add failed: %d\n", millis(), addResult);
      return;
    }
  }

  esp_err_t result = esp_now_send(nodeMac, ack, sizeof(ack));
  Serial.printf("[%lu ms] ACK SENT, result=%d, sequence=%lu\n",
                millis(), result, sequence);
}

void onDataReceive(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  Serial.println();
  Serial.println("----------------------------------------");
  Serial.printf("[%lu ms] PACKET RECEIVED\n", millis());

  Serial.print("Node MAC: ");
  printMac(info->src_addr);
  Serial.println();

  Serial.printf("Length: %d bytes\n", len);

  if (len != 23 || data[0] != 0x01) {
    Serial.println("Packet is not the expected telemetry format.");
    return;
  }

  Telemetry t{};
  memcpy(&t.type, data, 1);
  memcpy(&t.sequence, data + 1, 4);
  memcpy(&t.fingerprint, data + 5, 4);
  memcpy(&t.previousWakeDurationMs, data + 9, 4);
  memcpy(&t.batteryMv, data + 13, 2);
  memcpy(&t.port1Mv, data + 15, 2);
  memcpy(&t.port2Mv, data + 17, 2);
  memcpy(&t.port3Mv, data + 19, 2);
  memcpy(&t.port4Mv, data + 21, 2);

  Serial.printf("Sequence: %lu\n", t.sequence);
  Serial.printf("Fingerprint: 0x%08lX\n", t.fingerprint);
  Serial.printf("Previous wake: %lu ms\n", t.previousWakeDurationMs);
  Serial.printf("Battery: %u mV\n", t.batteryMv);
  Serial.printf("Port 1: %u mV\n", t.port1Mv);
  Serial.printf("Port 2: %u mV\n", t.port2Mv);
  Serial.printf("Port 3: %u mV\n", t.port3Mv);
  Serial.printf("Port 4: %u mV\n", t.port4Mv);

  int rssi = info->rx_ctrl ? info->rx_ctrl->rssi : 0;
  Serial.printf("RSSI: %d dBm\n", rssi);

  // Immediate ACK. No Google/script/update processing in this bench build.
  sendAck(info->src_addr, t.sequence);

  Serial.println("----------------------------------------");
}

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println();
  Serial.println("========================================");
  Serial.println(" KREK LABS - XIAO ESP32-C6 HUB BENCH");
  Serial.println("========================================");

  WiFi.mode(WIFI_STA);
  delay(50);

  Serial.print("Hub MAC: ");
  Serial.println(WiFi.macAddress());

  esp_err_t channelResult =
    esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);

  Serial.printf("ESP-NOW channel %d, result=%d\n",
                ESPNOW_CHANNEL, channelResult);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW INIT FAILED");
    while (true) delay(1000);
  }

  esp_now_register_recv_cb(onDataReceive);

  Serial.println("ESP-NOW READY");
  Serial.println("Waiting for Node telemetry...");
}

void loop() {
  delay(100);
}
