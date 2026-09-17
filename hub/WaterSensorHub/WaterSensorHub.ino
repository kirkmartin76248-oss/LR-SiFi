#include "HubConfig.h"
#include "HubStorage.h"
#include "LoRaHub.h"
#include "Cloud.h"

HubConfig cfg;
HubStorage storage;
LoRaHub lora;
Cloud cloud;

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
