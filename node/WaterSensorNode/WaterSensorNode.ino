#include <Arduino.h>
#include "Config.h"
#include "NodeConfig.h"
#include "Sensors.h"
#include "LoRaRadio.h"
#include "Protocol.h"
#include "PowerManager.h"
#include "BleConfig.h"

NodeConfig cfg;
LoRaRadio radio;
NodeBleConfig ble;
uint32_t sequenceNo = 0;

static bool enterBleIfRequested(bool coldBoot) {
  if (!coldBoot) return false;

  // GPIO9 is the XIAO ESP32-C6 BOOT button. Do not hold it during reset;
  // GPIO9 is also a boot-strapping pin. Press it after the application starts.
  pinMode(9, INPUT);
  uint32_t start = millis();
  while (millis() - start < 2000UL) {
    if (digitalRead(9) == LOW) {
      ble.begin(cfg);
      while (ble.active()) {
        ble.loop(cfg);
        delay(10);
      }
      return true;
    }
    delay(10);
  }
  return false;
}

void setup() {
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LED_OFF);

  Serial.begin(115200);
  delay(40);

  loadConfig(cfg);
  initSensors();
  initBatteryADC();

  bool radioWake = (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT1);

  if (enterBleIfRequested(!radioWake)) {
    digitalWrite(PIN_LED, LED_OFF);
    ESP.restart();
  }

  pinMode(PIN_RADIO_DIO1, INPUT);

  if (!radio.begin(cfg)) {
    digitalWrite(PIN_LED, LED_ON);
    delay(500);
    goToDeepSleep();
  }

  digitalWrite(PIN_LED, LED_ON);

  if (radioWake) {
    handleRadioWake();
  } else {
    radio.startDutyCycleRX();
  }

  digitalWrite(PIN_LED, LED_OFF);
  goToDeepSleep();
}

void loop() {
}

void handleRadioWake() {
  PacketHeader hdr{};
  PollPacket poll{};

  if (!radio.receivePoll(hdr, poll)) {
    radio.startDutyCycleRX();
    return;
  }

  if (hdr.networkId != cfg.networkId ||
      hdr.hubId != cfg.hubId ||
      hdr.nodeId != cfg.nodeId ||
      hdr.type != PKT_POLL) {
    radio.startDutyCycleRX();
    return;
  }

  sensorsPower(true);
  delay(SENSOR_SETTLE_MS);

  TelemetryPacket t{};
  t.version = PROTOCOL_VERSION;
  t.type = PKT_TELEMETRY;
  t.networkId = cfg.networkId;
  t.hubId = cfg.hubId;
  t.nodeId = cfg.nodeId;
  t.sequence = ++sequenceNo;

  readTelemetry(t);

  sensorsPower(false);
  radio.sendTelemetry(t);

  AckPacket ack{};
  if (radio.receiveAck(ack, ACK_TIMEOUT_MS)) {
    if (ack.networkId == cfg.networkId &&
        ack.hubId == cfg.hubId &&
        ack.nodeId == cfg.nodeId &&
        ack.sequence == t.sequence) {

      if (ack.configVersion != cfg.configVersion) {
        cfg.reportIntervalSec = ack.reportIntervalSec;
        cfg.txPowerDbm = ack.txPowerDbm;
        cfg.configVersion = ack.configVersion;
        saveConfig(cfg);
        radio.applyConfig(cfg);
      }
    }
  }

  radio.startDutyCycleRX();
}
