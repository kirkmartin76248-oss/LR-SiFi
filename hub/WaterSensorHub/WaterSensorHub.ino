#include "HubConfig.h"
#include "HubStorage.h"
#include "LoRaHub.h"
#include "Cloud.h"
#include "BleConfig.h"

HubConfig cfg;
HubStorage storage;
LoRaHub lora;
Cloud cloud;
HubBleConfig ble;

static bool enterBleIfRequested() {
  // GPIO9 is the XIAO ESP32-C6 BOOT button. Do not hold it during reset;
  // GPIO9 is a boot-strapping pin. Press it after the application starts.
  pinMode(9, INPUT);
  uint32_t start = millis();
  while (millis() - start < 2000UL) {
    if (digitalRead(9) == LOW) {
      ble.begin(cfg, storage);
      while (ble.active()) {
        ble.loop(cfg, storage);
        delay(10);
      }
      ESP.restart();
      return true;
    }
    delay(10);
  }
  return false;
}

void setup() {
  Serial.begin(115200);
  delay(300);

  storage.begin();
  storage.loadConfig(cfg);

  Serial.println();
  Serial.println("Water Sensor Hub Rev 1");
  Serial.println("----------------------");
  Serial.printf("Hub ID: %s\n", cfg.hubId.c_str());
  Serial.printf("Poll interval: %lu s\n", (unsigned long)cfg.pollIntervalSec);

  if (enterBleIfRequested()) return;

  if (!lora.begin(cfg)) {
    Serial.println("LoRa init FAILED");
    while (true) delay(1000);
  }
  cloud.begin(cfg);
}

void loop() {
  for (uint8_t i = 0; i < cfg.nodeCount; ++i) {
    if (!cfg.nodes[i].enabled) continue;

    Telemetry t;
    bool ok = lora.pollNode(cfg.nodes[i], t, cfg);
    if (ok) {
      Serial.printf("Node %u: TEMP=%ldmV DO=%ldmV WATER=%d AIR=%d BAT=%ldmV RSSI=%d SNR=%d\n",
                    t.nodeId, t.tempMv, t.doMv, t.waterFlow, t.airFlow,
                    t.batteryMv, t.rssi, t.snr);
      cloud.sendTelemetry(t, cfg);
    } else {
      Serial.printf("Node %u: poll timeout/no valid response\n", cfg.nodes[i].nodeId);
    }
    delay(50);
  }

  cloud.refreshConfig(cfg, storage);
  delay(100);
}
